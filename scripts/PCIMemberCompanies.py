#!/usr/bin/env python3
# -*- coding: utf-8 -*-
# ------------------------------------------------------------------------
#   File Name: PCIMemberCompanies.py
#      Author: Zhao Yanbai
#              2021-11-11 22:00:59 Thursday CST
# Description: none
# ------------------------------------------------------------------------
import re
import requests
from bs4 import BeautifulSoup

members = {}

for page in range(0, 10) :
    rsp = requests.get("https://pcisig.com/membership/member-companies?page={0}".format(page))
    soup = BeautifulSoup(rsp.text , "html.parser")

    tds = soup.find_all("td", attrs = {"class" : "views-field"})


    for i in range(0, len(tds), 2) :
        ids = tds[i+1].text.strip()
        name = tds[i+0].text.strip()
        name = name.replace("\"", "\\\"")

        if len(ids) == 0 :
            continue

        ids = ids.split('(')
        if len(ids) != 2 :
            continue

        ids = ids[0]
        ids = int(ids)

        members[ids] = name


for ids in sorted(members.keys()) :
    print("""{{0x{0:X}, "{1}"}},""".format(ids, members[ids]))
