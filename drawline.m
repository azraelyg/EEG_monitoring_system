function drawline( handles,xaxis,rawData,rawTime,samplerate)
range=xaxis/samplerate; % time range
x=0:1/samplerate:range; %x range
x=x(2:end);  %skip 0
range_x=0:range/5:range; %x axis range

axes(handles.ch1);%channel 1
plot(x,rawData(1:xaxis,1),'-');
xlabel('time'); %set lael
set(gca,'XTick',range_x);
range_time=round(range_x*samplerate);
set(gca,'XTickLabel',[char(rawTime(1,:));char(rawTime(range_time(2:end),:)) ]); %label time


axes(handles.ch2);%channel 2
plot(x,rawData(1:xaxis,2),'-');
xlabel('time');
set(gca,'XTick',range_x);
range_time=round(range_x*samplerate);
set(gca,'XTickLabel',[char(rawTime(1,:));char(rawTime(range_time(2:end),:)) ]);


axes(handles.ch3);%channel 3
plot(x,rawData(1:xaxis,3),'-');
xlabel('time');
set(gca,'XTick',range_x);
 range_time=round(range_x*samplerate);
set(gca,'XTickLabel',[char(rawTime(1,:));char(rawTime(range_time(2:end),:)) ]);


axes(handles.ch4);%channel 4
plot(x,rawData(1:xaxis,4),'-');
xlabel('time');
set(gca,'XTick',range_x);
range_time=round(range_x*samplerate);
set(gca,'XTickLabel',[char(rawTime(1,:));char(rawTime(range_time(2:end),:)) ]);


end

