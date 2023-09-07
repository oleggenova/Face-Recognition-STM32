% Kernel function for 'Face Detection on ARM Target using Code Generation' example

function rows = my_ViolaJones(inGray)

%#codegen
persistent faceDetector


% Instantiate system object
if isempty(faceDetector)
    faceDetector = vision.CascadeObjectDetector('MinSize', [20 20], ...
        'MaxSize', [240 320]);
end


% Create uninitialized memory in generated code
%outRGB = coder.nullcopy(inGray);

% Detect faces and create boundiong boxes around detected faces
bbox = single(step(faceDetector, inGray));
[rows,~] = size(bbox);

% Limit the number of faces to be detected in an image.  insertShape
% requires that bbox signal must be bounded
%assert(size(bbox, 1) < 10);

% Insert rectangle shape for bounding box
%outRGB(:) = insertShape(inGray, 'Rectangle', bbox);

end