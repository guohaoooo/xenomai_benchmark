#!/bin/sh

sudo ./sem_post_wait_inter_process c &
sleep 1
sudo ./sem_post_wait_inter_process  &
