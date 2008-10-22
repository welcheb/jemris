function hpt=seqcad_uitoolbar(hObject,handles)

%TS@IME-FZJ 03/2007
ht = uitoolbar(hObject);
ic=handles.icons;
Modules=handles.Modules;
ModuleToolTip=Modules;

%draw buttons for all modules
NM=length(handles.Modules);
for i=1:NM
    %find icon for this module
    try
     eval(['B=ic.',Modules{i},';']);
    catch
        if findstr(Modules{i},'SEQ')
            B=ic.ANYSEQUENCE;
        else
            B=ic.ANYPULSE;
        end
    end
    %draw separator after this module for SEQ|PULSES
    bsep='off';
    if (i>1)
        if ( ~isempty(findstr(Modules{i-1},'SEQ')) && ~isempty(findstr(Modules{i},'PULSE')) ) ...
           || ( isempty(findstr(Modules{i-1},'RF')) && ~isempty(findstr(Modules{i},'RF')) ) ...
           || ~isempty(findstr(Modules{i-1},'EMPTY'))
            bsep='on';
        end
    end
    %make background white for pulse icons
    if findstr(Modules{i},'PULSE'); B(find(isnan(B)))=1; end
    %draw button
    handles.hpt{i} = uitoggletool(ht,'CData',B,'TooltipString',['Insert ',ModuleToolTip{i}],'Separator',bsep);
end

%draw buttons for ERASE, COPY, SWAP modules
handles.hpt{NM+1} = uitoggletool(ht,'CData',ic.ERASEMODULE,'TooltipString','erase a module','Separator','on');
COPYMODULE=permute(ic.SWAPMODULES,[2 1 3]);
handles.hpt{NM+2} = uitoggletool(ht,'CData',COPYMODULE,'TooltipString','copy a module','Separator','on');
handles.hpt{NM+3} = uitoggletool(ht,'CData',ic.SWAPMODULES,'TooltipString','swap two modules','Separator','on');

%define callbacks
for i=1:NM
    set(handles.hpt{i},'OnCallback',{@tbbdf_common,Modules{i},handles});
end
set(handles.hpt{NM+1},'OnCallback',{@tbbdf_Erase,handles});
set(handles.hpt{NM+2},'OnCallback',{@tbbdf_Copy,handles});
set(handles.hpt{NM+3},'OnCallback',{@tbbdf_Swap,handles});
for i=1:length(handles.hpt)
    set(handles.hpt{i},'OffCallback',@tbbdf_Off);
end

hpt =handles.hpt;

%%%%%%%%%%%%%%%%%%%%%&%%%%%%%%%%%
%%%%%%  callback functions %%%%%%
%%%%%%%%%%%%%%%%%%%%%%&%%%%%%%%%%
function tbbdf_Erase(src,eventdata,handles)
 tbbdf_common([ ], [ ] ,[ ],handles)
 global INSERT_MODULE_NUMBER
 INSERT_MODULE_NUMBER=-1;

function tbbdf_Swap(src,eventdata,handles)
 tbbdf_common([ ], [ ] ,[ ],handles)
 global INSERT_MODULE_NUMBER MODULE1 MODULE2
 INSERT_MODULE_NUMBER=-2; MODULE1=0; MODULE2=0;

function tbbdf_Copy(src,eventdata,handles)
 tbbdf_common([ ], [ ] ,[ ],handles)
 global INSERT_MODULE_NUMBER MODULE1 MODULE2
 INSERT_MODULE_NUMBER=-3; MODULE1=0; MODULE2=0;

function tbbdf_Off(src,eventdata)
 global INSERT_MODULE_NUMBER MODULE1 MODULE2
 INSERT_MODULE_NUMBER=0; MODULE1=0; MODULE2=0;

%all callbacks call this routine
function tbbdf_common(src,eventdata,seq,handles)
 global INSERT_MODULE_NUMBER
 for i=1:length(handles.hpt)
    if (gcbo~=handles.hpt{i})
        set(handles.hpt{i},'State','off') %switch off all other modules
    else
        imn=i;
    end
 end
 INSERT_MODULE_NUMBER=imn;           %set current module active for insertion
 guidata(handles.output, handles);

