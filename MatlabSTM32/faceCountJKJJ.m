%%Detect n faces in the given working directory
%Created By: J. K. Josephine Julina
clear all;
close all;
clc;
srcFiles = dir([pwd '\*.jpg']);
 for i = 1 : length(srcFiles) 
     filename= strcat([pwd '\'] ,srcFiles(i).name);
     image = imread(filename); 
     image = rgb2gray(image);
     image = imresize(image,[900 900]);     
     faceDetector = vision.CascadeObjectDetector();
     fbox = step(faceDetector,image); 
       [m,n] =  size(int32(fbox));         
     fout = insertObjectAnnotation(image,'rectangle',fbox,'Face');
     figure,  subplot(1,2,1);imshow(image);title('Input Image');
      fout = insertText(fout, [20 20], m, 'BoxOpacity', 1, ...
        'FontSize', 14);   
     subplot(1,2,2);imshow(fout);title('Detected Faces');   
     uiwait(msgbox(sprintf('Face Count = %d',m),'Detected Faces','modal'));     
 end
close all;