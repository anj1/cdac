n = 2000;
x = linspace(0,2*pi,n);
r = 11*(exp(-x*5) + exp(5*(-2*pi + x))) + 0.25*cos(x*20) + 0.2;

xo = x - pi;
r = r - exp(-xo.*xo*4);

plot(x,r);
pause();
f = abs(fft(r));
plot(x,f);
pause();
p = 2*pi*rand(1,n);
f = f .* exp(i*p);
d = real(ifft(f));
polar(x,d+50);
%hold on;
%polar(x,0.0*d + 50);
pause();
