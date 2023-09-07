% immagine 76800 YUV

matrice = zeros(240,320,'uint8');

frames = uint8([serialport_data5;serialport_data6;serialport_data7;serialport_data9]);
%%
for j = 1:height(frames)
    matrice = reshape(frames(j,:),320,240)';
    subplot(2,2,j)
    imshow(matrice)
    title("Frame " + j)
end
