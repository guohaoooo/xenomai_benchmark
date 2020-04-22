#!/bin/sh

sudo ./named_sem_post_wait_inter_process c &
sleep 1
sudo ./named_sem_post_wait_inter_process  &
