#! /usr/bin/octave -qf
arg_list = argv();
extr = load(arg_list{1});
n = input("enter number of histogram bins: ");
hist(extr,n);
sqrt(var(extr))
mean(extr)
waitforbuttonpress
