h = figure;
set (h,'papertype', '<custom>')
set (h,'paperunits','centimeters');
set (h,'papersize',[5.2 3.7])
set (h,'paperposition', [0,0,[5.2 3.7]])
set (h,'defaultaxesposition', [0.15, 0.15, 0.75, 0.75])
set (h,'defaultaxesfontsize', 14)

A = load("autocorr.dat");
size(A)
x = 1:size(A,2);
x = x/(x(size(x,2))/(2*pi));	% make a nice plot honey
A = sum(A);
plot(x,A);
title('Autocorrelation');
xlabel('Offset');
%pause();

print('smac.eps','-deps')

