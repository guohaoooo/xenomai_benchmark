#!/bin/sh


sudo ./yield_inter_process &
sleep 1
sudo ./yield_inter_process c &

