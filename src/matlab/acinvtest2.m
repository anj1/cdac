#!/usr/bin/env octave

g = figure;
set (g,'papertype', '<custom>')
set (g,'paperorientation', 'landscape')
set (g,'paperunits','centimeters')
set (g,'papersize',[5.2 3.7])
set (g,'paperposition', [0,0,[5.2 3.7]])
set (g,'defaultaxesposition', [0.15, 0.15, 0.75, 0.75])
set (g,'defaultaxesfontsize', 14)

n = 2000;
x = linspace(0,2*pi,n);
r = 11*(exp(-x*5) + exp(5*(-2*pi + x))) + 0.25*cos(x*20) + 0.2;

xo = x - pi;
r = r - 2*exp(-xo.*xo*4);

plot(x,r,'-',linewidth=2.0);
print('../doc/img/art-ac.eps','-deps')

f = abs(fft(r));
p = 2*pi*rand(1,n);
f = f .* exp(i*p);
d = real(ifft(f));

%plot(x(1:100),f(1:100));
%pause();

h = figure;
set (h,'papertype', '<custom>')
set (h,'paperunits','centimeters')
set (h,'papersize',[3.7 3.7])
set (h,'paperposition', [0,0,[3.7 3.7]])
set (h,'defaultaxesposition', [0.1, 0.1, 0.8, 0.8])
set (h,'defaultaxesfontsize', 14)

h2=polar(x,d+50);
set (h2,"linewidth",2.0);

print('../doc/img/test01.eps','-deps')
