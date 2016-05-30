function drawline_d( handles,i,x,range,rawData )
axes(handles.ch1);%channel 1
plot(x(1:i),rawData(1:i,1),'-');
xlim([0 range]);
xlabel('time(s)');
axes(handles.ch2); %channel 2
plot(x(1:i),rawData(1:i,2),'-');
xlim([0 range]);
xlabel('time(s)');

axes(handles.ch3);%channel 3
plot(x(1:i),rawData(1:i,3),'-');
xlim([0 range]);
xlabel('time(s)');

axes(handles.ch4);%channel 4
plot(x(1:i),rawData(1:i,3),'-');
xlim([0 range]);
xlabel('time(s)');


end

