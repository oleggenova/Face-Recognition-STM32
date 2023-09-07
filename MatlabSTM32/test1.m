% leggo immagine 320x240

inRGB = imread("2.jpg");
inRGB = imresize(inRGB,[240 320]);

% converto in monocrhrome gray uint8
inGray = rgb2gray(inRGB);
imshow(inGray)

rows = my_ViolaJones(inGray);

disp("Numero facce:")
disp(rows)


