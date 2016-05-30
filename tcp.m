function [rawTime,rawData,recv] = tcp( ip )

i=1;
msg_size=68;
rawTime=[];
rawData=[];
sendtobuffer=['read',' 3 1'];
tcpipClient=tcpip(ip,2000,'NetworkRole','Client');
set(tcpipClient,'InputBufferSize',msg_size);
set(tcpipClient,'Timeout',30);
fopen(tcpipClient);
fwrite(tcpipClient,sendtobuffer,'char');
recv=char(fread(tcpipClient,msg_size,'char'));

while(recv(1)~='0')

    rawTime(i,:)=fread(tcpipClient,msg_size,'char');
    fwrite(tcpipClient,'go','char');
    
    temp=char(fread(tcpipClient,msg_size,'char'));
    rawData(i,:)=sscanf(temp,'%f')';
    fwrite(tcpipClient,'go','char');
    i=i+1;
    
    recv=char(fread(tcpipClient,msg_size,'char'));
    fwrite(tcpipClient,'go','char');
end
    fclose(tcpipClient);
end

