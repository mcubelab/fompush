function [] = LPPlot(plot_data, flag)
fric = LPusher.Friction;
if flag == 0
    index = [1;2];
elseif flag == 1
    index = [3,4];
elseif flag == 2
    index = [5];
end
    Name = 'test';
    Figures.(Name)=Figure;
    Figures.(Name).filename = Name;  
    Figures.(Name).Create(length(index),1); 
    
    if flag==0
        Figures.(Name).xData = {[plot_data.t, plot_data.t];[plot_data.t, plot_data.t]};
        Figures.(Name).yLabel={'$f_{n1}$ (N)';'$f_{n2}$ (N)'}; 
        Y1  = plot_data.uc{2}(:,index(1))*0;
        Y2  = plot_data.uc{2}(:,index(1))*0;
        Figures.(Name).yData = {[Y1,plot_data.uc{2}(:,index(1))];[Y2,plot_data.uc{2}(:,index(2))]};
        Figures.(Name).xLabel={'t(s)';'t(s)'};
        Figures.(Name).Color = {['k','b'];['k','r']};
    elseif flag==1
        Figures.(Name).xData = {[plot_data.t, plot_data.t, plot_data.t];[plot_data.t, plot_data.t, plot_data.t]};
        Figures.(Name).yLabel={'$f_{t1}$ (N)';'$f_{t2}$ (N)'}; 
        Y1  = plot_data.uc{2}(:,1)*fric.nu_p;
        Y2  = -plot_data.uc{2}(:,1)*fric.nu_p;
        Y3  = plot_data.uc{2}(:, 2)*fric.nu_p;
        Y4  = -plot_data.uc{2}(:,2)*fric.nu_p;
        Figures.(Name).yData = {[Y1,Y2,plot_data.uc{2}(:,index(1))];[Y3,Y4,plot_data.uc{2}(:,index(2))]};
        Figures.(Name).xLabel={'t(s)';'t(s)'};
        Figures.(Name).Color = {['k','k','b'];['k','k','r']};
    else
        Figures.(Name).xData = {[plot_data.t, plot_data.t]};
        Figures.(Name).yLabel={'$dot{r}_y$ (m/s)'}; 
        Y1  = plot_data.uc{2}(:,5)*0;
        Figures.(Name).yData = {[Y1,plot_data.uc{2}(:,index(1))]};
        Figures.(Name).xLabel={'t(s)'};
        Figures.(Name).Color = {['k','b']};
    end
    
    Figures.(Name).Title = {Name};               
    Figures.(Name).Plot2d;
%     Figures.(Name).Save(Foldername);
end
