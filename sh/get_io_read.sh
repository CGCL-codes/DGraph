#! /bin/bash
#get now IO read volume (KB)

#iostat -d -k
iostat -d -k |grep sda | awk '{print $5}'

