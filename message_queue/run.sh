#!/bin/sh

sudo ./mq_send_receive_process  &
sleep 1
sudo ./mq_send_receive_process c  &
