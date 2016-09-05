#! /bin/bash
#get now IO write volume (KB)

#iostat -d -k


iostat -d -k |grep sda | awk '{print $6}'


