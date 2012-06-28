a = load("autocorr.dat");

g = figure;
set (g,'papertype', '<custom>')
set (g,'paperorientation', 'landscape')
set (g,'paperunits','centimeters')
set (g,'papersize',[5.2 3.7])
set (g,'paperposition', [0,0,[5.2 3.7]])
set (g,'defaultaxesposition', [0.15, 0.15, 0.75, 0.75])
set (g,'defaultaxesfontsize', 14)

x = linspace(0,2*pi,size(a,2));

for i = 1 : size(a,1)
	plot(x,a(i,:));
	print(sprintf('ac-%04d.eps',i),'-deps');
end

# variance
plot(x,sum(a)/9.0,'r',x,sqrt(var(a)),'.');
print('ac-var.eps','-deps');
