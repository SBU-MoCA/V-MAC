#!/bin/bash


sudo rmmod ath9k_htc
sudo rmmod ath9k_common
sudo rmmod ath9k_hw
sudo rmmod ath
sudo rmmod mac80211
sudo insmod ath.ko
sudo insmod vmac.ko
sudo insmod ath9k_hw.ko
sudo insmod ath9k_common.ko
sudo insmod ath9k_htc.ko
