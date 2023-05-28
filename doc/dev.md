# 开发日志

这个文件主要用来记录特定版本的代码存在的主要问题，及修改方向。

## 18cf417585b83e74d413f247fd578f2654e8c177 版本问题

目前中断下半部处理有一个问题：就是第一个中断正在进行下半部分开中断处理的情况下，被第二个中断处理中断后，如果第二个中断处理有下半部处理需求，是无法得到调用的。因为第二个中断处理会直接返回不会调用下半部处理逻辑。

## 92ca4828ee74f800ba3bdd132f162a739075d9f2 版本问题

这个版本的问题表现是运行一段时间后，会在`clock_bh_handler`的`assert(p->state == TASK_WAIT);`处断言失败。经分析问题出在`sysc_wait`处。

需要指明的前提是，目前版本代码在系统调用中上下文中还没有添加抢占相关的逻辑。


```c
128 int sysc_wait(unsigned long cnt) {
129     unsigned long flags;
130     irq_save(flags);
131     current->state = TASK_WAIT;
132     current->delay_jiffies = jiffies + cnt;
133     list_add(&current->pend, &delay_tasks);
134     irq_restore(flags);
135 
136     schedule();
137 }
```

```c
182 void schedule() {
        ......
193     unsigned long iflags;
194     irq_save(iflags);
195 
196     if (0 == current->ticks) {
197         current->turn++;
198         current->ticks = current->priority;
199         current->state = TASK_READY;
200     }
```

这段代码在第`sysc_wait:134`行开中断，在`schedule:193`处再关中断。那么中断程序完全可能在这两个点之间中断这个逻辑，再叠加上ticks被减到`0`需要重新调度的条件就会出现该问题。

问题路径：

1. `current->ticks == 1`
2. 程序执行到`sysc_wait:134`和`schedule:193`之间
3. 时钟中断到达且时钟中断没有嵌套在其它中断的下半部分逻辑中
4. 时钟中断将`current-ticks`减`1`至`0`
5. 在时钟中断退出前调用`schedule`,
6. `schedule`在判断到`current->ticks == 0`时无视其`TASK_WAIT`的状态，将其设置为`TASK_READY`
7. 再次时钟中断到达，判断到睡眠队列里出现了`TASK_READY`状态的任务。

### 修改方案1

修改逻辑为在`schedule`完成前，不允许中断。

```c
int sysc_wait(unsigned long cnt) {
    unsigned long flags;
    irq_save(flags);
    current->state = TASK_WAIT;
    current->delay_jiffies = jiffies + cnt;
    list_add(&current->pend, &delay_tasks);

    schedule();

    irq_restore(flags);
}
```

### 修改方案2

在`schedule`的逻辑加入只对`TASK_RUNNING`改成`TASK_READY`。

```c
    if (0 == current->ticks) {
        current->turn++;
        current->ticks = current->priority;
    }

    if (current->state == TASK_RUNNING) {
        current->state = TASK_READY;
    }
```

目前准备采用方案2修复。

## 001073af2176de114d8588124d665aad2b4f2995 版本问题

### 问题表现

虽然在`kernel/sched.c:schedule`里如果`0 == current->ticks`则会将其重置的逻辑，但在`clock_handler`里的`current->ticks-`代码任有可能将其值减到0后继续再减进而溢出。就算是`schedule`代码里的处理逻辑已经是在关中断的情况下进行的也无济于事。

将`qemu`的速度调至10%比较容易复现这个问题。

### 问题原因

在这个版本中，由于在`clock_handler`里塞了过多的调试代码，导致其耗时很长。这就可能会出现如下处理序列：

1. 处理器进入硬盘`14`号中断的处理逻辑
2. 如果硬盘中断逻辑已经到达开中断的部分
3. 这时时钟中断到达，则会中断当前硬盘中断转而执行时钟中断。
4. 时钟中断执行`current->ticks--`，其值有可能从`1`变为`0`
5. 时钟中断占用相当长时间，甚至超过两次时钟中断间隔
6. 时钟中断返回前判断到当前中断处于嵌套状态，不执行重新调度，此时`current->ticks == 0`。
7. 时钟中断返回
8. 由于上一个时钟中断耗时过长，因此当其刚返回后，14号中断还没来得及继续执行，或者刚执行了有限的几条指令，又被下一个时钟中断打断。
9. 重复以上逻辑，这个时钟中断逻辑就继续对`current->ticks`减`1`，使其值为`0xFFFFFFFF`。

### 问题修复计划

### 优化中断处理服务程序

只写少量必要代码，尽快返回，不再加入大量调试程序。

#### 不再支持中断嵌套

不支持中断嵌套仅指不嵌套执行中断服务器程序，而其它需要在中断环境执行的逻辑是可以开中断执行的，也就是有再次被新中断打断的可能。

不再支持中断嵌套后，每次中断处理过程大致如下

1. 硬件触发中断，自动关中断
2. 保存上下文
3. 执行中断服务例程
3. 开中断
4. 执行其它逻辑
5. 关中断
6. 恢复上下文返回

可以到第4步是在开中断的状态下执行的。而执行它的时候是可能被新到的中断程序再次打断的，也就是第4步是可能嵌套执行的。

为了避免这种情况，将上述程序引入一个`int`型的全局变更`reenter`，并将其初始化为`-1`后再做修改

1. 硬件触发中断，自动关中断
2. 保存上下文
3. `reenter++`
4. 执行中断服务例程
5. 判断`reenter != 0`则跳转到第`9`步
6. 开中断
7. 执行其它逻辑
8. 关中断
9. `reenter--`
10. 恢复上下文返回

如果在执行第`7`步后被新到中断打断，那新逻辑就会执行如下序列


1. 硬件触发中断，自动关中断
2. 保存上下文
3. `reenter++`
4. 执行中断服务例程
5. 判断`reenter != 0`则跳转到第`9`步
6. ~~开中断~~
7. ~~执行其它逻辑~~
8. ~~关中断~~
9. `reenter--`
10. 恢复上下文返回

这样就算第`7`步很耗时，在中断打断这个步骤的情况下，它也只会被执行一次，而不会嵌套执行。这是优点，也是缺点，目前逻辑简单还能应付。后续也还是要继续优化。


但这也改变不了时钟中断可能将`current->ticks`改成负数的情况（因为可能在`current->ticks == 0`的情况下，第`7`步是有可能再撑到下一个时钟中断将之再减1的）。因此需要在`task_union`中引入一个`need_resched`字段，当`current->ticks == 0`时不再减，而是直接将`need_resched`设置为`1`。


1. 硬件触发中断，自动关中断
2. 保存上下文
3. `reenter++`
4. 执行中断服务例程
5. 判断`reenter != 0`则跳转到第`11`步
6. 开中断
7. 执行其它逻辑
8. 关中断
9. assert(reenter == 0);
10. 如果`current->need_resched == 0`则继续，否则跳`20`
11. `reenter--`
12. 恢复上下文返回


以下为需要进程抢占步骤，目前不分内核还是用户进程。

20. `reenter--`、`current->need_resched = 0`、`current->ticks = current->priority`
21. schedule
22. 跳`12`

相似的逻辑也可以加到系统调用返回处。这样就可以在中断返回和系统调用返回处抢占任务。