function varargout = gui(varargin)

% Begin initialization code - DO NOT EDIT
gui_Singleton = 1;
gui_State = struct('gui_Name',       mfilename, ...
                   'gui_Singleton',  gui_Singleton, ...
                   'gui_OpeningFcn', @gui_OpeningFcn, ...
                   'gui_OutputFcn',  @gui_OutputFcn, ...
                   'gui_LayoutFcn',  [] , ...
                   'gui_Callback',   []);
if nargin && ischar(varargin{1})
    gui_State.gui_Callback = str2func(varargin{1});
end

if nargout
    [varargout{1:nargout}] = gui_mainfcn(gui_State, varargin{:});
else
    gui_mainfcn(gui_State, varargin{:});
end
% End initialization code - DO NOT EDIT


% --- Executes just before gui is made visible.
function gui_OpeningFcn(hObject, eventdata, handles, varargin)
% Choose default command line output for gui
handles.output = hObject;
% Update handles structure
guidata(hObject, handles);


% --- Outputs from this function are returned to the command line.
function varargout = gui_OutputFcn(hObject, eventdata, handles) 
varargout{1} = handles.output;


function T_usrno_Callback(hObject, eventdata, handles) %user no text box
usrno=str2double(get(hObject,'String')); %get user no
if isnan(usrno)
    set(hObject, 'String', 0);
    errordlg('Input must be a number','Error');
end

handles.metricdata.T_usrno = usrno;
guidata(hObject,handles)

% --- Executes during object creation, after setting all properties.
function T_usrno_CreateFcn(hObject, eventdata, handles)
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end

function T_seq_Callback(hObject, eventdata, handles) %sequence no text box
sequence=str2double(get(hObject,'String'));%get sequence no
if isnan(sequence)
    set(hObject, 'String', 0);
    errordlg('Input must be a number','Error');
end
handles.metricdata.T_seq = sequence;
guidata(hObject,handles)

% --- Executes during object creation, after setting all properties.
function T_seq_CreateFcn(hObject, eventdata, handles)
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end

% --- Executes on button press in B_Inquiry.
function B_Inquiry_Callback(hObject, eventdata, handles)
msg_size=68;
%get usrno sequence
sequence=handles.metricdata.T_seq;%sequencd
usrno=handles.metricdata.T_usrno;%user number
ip=handles.metricdata.T_ip; %ip
port=handles.metricdata.T_port;%port

%%%%set tcp%%%%%%%%%%%%5
tcpipClient=tcpip(ip,port,'NetworkRole','Client');
set(tcpipClient,'InputBufferSize',msg_size);
set(tcpipClient,'Timeout',30);
fopen(tcpipClient);

%first socket message 
sendtobuffer=['read',' ',int2str(usrno),' ',int2str(sequence)];  %"read"+usrno+sequence
fwrite(tcpipClient,sendtobuffer,'char'); %send
%%%%%%%%%%receive time%%%%%%%%%%%%
recv=char(fread(tcpipClient,msg_size,'char')); %wait confirm
if (recv(1)~='n')
    str = deblank(recv');
    str = regexp(str, ',','split'); %split the receving data
    starttime=char(str(1)); %start time
    endtime=char(str(2));%end time
    set(handles.L_starttime,'string',starttime);
    set(handles.L_endtime,'string',endtime);
    set(handles.T_starttime,'string',starttime); %default starttime is the starttime of signal
else
    errordlg('no record','Error');
end
handles.tcp=tcpipClient;
guidata(hObject,handles)




function T_starttime_Callback(hObject, eventdata, handles) %start time text box
starttime=get(hObject,'String');
handles.metricdata.T_starttime = starttime;
guidata(hObject,handles)

% --- Executes during object creation, after setting all properties.
function T_starttime_CreateFcn(hObject, eventdata, handles)

if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end


% --- Executes on button press in B_update.
function B_update_Callback(hObject, eventdata, handles) %update button ,
                                                        %choose the time
                                                        %and receive data
%send signal
tcpipClient=handles.tcp;
i=1; %count receive data number
samplerate=128;
msg_size=68; %message size
range=str2double(handles.metricdata.P_range); %time range
xaxis=samplerate*range; %get data number 128 samples per second
%x=0:1/samplerate:range; %x range comment if not dynamic plot
rawTime=zeros(xaxis,msg_size); %receiving data
rawData=zeros(xaxis,4);
%%%%%%send starttime and range%%%%%%%%%%
starttime=get(handles.T_starttime,'string'); %starttime
num=int2str(xaxis); %data number
fwrite(tcpipClient,[starttime,' ',num],'char');
%%%%%%%start receive data%%%%%%%%%
recv=char(fread(tcpipClient,msg_size,'char'));
if(recv(1)=='y')
    while(1)
        recv=char(fread(tcpipClient,msg_size,'char')); %if still has data
        if (recv(1)=='n')
            break; %transfer end
        end
        fwrite(tcpipClient,'go','char'); %reply to server

        rawTime(i,:)=fread(tcpipClient,msg_size,'char'); %time filed
        fwrite(tcpipClient,'go','char');

        temp=char(fread(tcpipClient,msg_size,'char')); %four channel fields
        rawData(i,:)=sscanf(temp,'%f')'; %raw data
        %drawline_d( handles,i,x,range,rawData ); %this is dynamic plot. t
                                                  %comment it and uncomment the drawline function
                                                   %below if not use
                                                   %dynamic

        fwrite(tcpipClient,'go','char'); 
        i=i+1;
    end
end
drawline( handles,i-1,rawData,rawTime,samplerate ); %use it if not dynamic
guidata(hObject,handles)
fclose(tcpipClient);

% --- Executes on selection change in P_range.
function P_range_Callback(hObject, eventdata, handles) %choose range

contents= cellstr(get(hObject,'String'));
range=contents{get(hObject,'Value')};
handles.metricdata.P_range = range;
guidata(hObject,handles)


% --- Executes during object creation, after setting all properties.
function P_range_CreateFcn(hObject, eventdata, handles) 

if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end
contents= cellstr(get(hObject,'String'));%get intial range, which is 10s
range=contents{get(hObject,'Value')};
handles.metricdata.P_range = range;
guidata(hObject,handles)


function T_ip_Callback(hObject, eventdata, handles)
ip=get(hObject,'String');
handles.metricdata.T_ip = ip;
guidata(hObject,handles)

% --- Executes during object creation, after setting all properties.
function T_ip_CreateFcn(hObject, eventdata, handles)

if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end


function T_port_Callback(hObject, eventdata, handles) %port text box
port=str2double(get(hObject,'String'));
if isnan(port)
    set(hObject, 'String', '2000');
    errordlg('Input must be an port','Error');
end
handles.metricdata.T_port = port;
guidata(hObject,handles)


% --- Executes during object creation, after setting all properties.
function T_port_CreateFcn(hObject, eventdata, handles)
if ispc && isequal(get(hObject,'BackgroundColor'), get(0,'defaultUicontrolBackgroundColor'))
    set(hObject,'BackgroundColor','white');
end
port=str2double(get(hObject,'String'));
handles.metricdata.T_port = port;
guidata(hObject,handles)
