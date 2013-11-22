openvibe-image-classes
======================

OpenViBE plugin for displaying images from different classes depending on the stimulus received. This is basically a modified and extended version of the DisplayCueImage plugin packaged with OpenViBE.



Installation instruction
------------------------
- Copy the files to your openvibe plugin directory
- Edit plugins/processing/simple-visualisation/src/ovp_main.cpp to add the following lines:
* In the headers: #include "box-algorithms/ovpCDisplayImageClasses.h"
* In the OVP_Declare: OVP_Declare_New(OpenViBEPlugins::SimpleVisualisation::CDisplayImageClassesDesc)
- Compile OpenViBE normally with script/linux-build

You should then get the box in Visualisation/Presentation


Use instructions
----------------
Add the box in the Designer, and connect it to a source of stimulations.

The module will present images on receiving stimulations. Each stimulation is linked with an "image class". For example, you could present images with a positive, neutral or negative emotional valence and link each one to an OpenViBE stimulation.

All of the images should be in the same directory and their name should match a pattern you can set in the box parameter.  ##CLASS## is replaced by the class number and ##NUMBER## by the image number in the class (Both start at 1). The default is img-##CLASS##-##NUMBER##.png 

The images are presented sequentially in each class. If Class 1 is linked to OVTK_StimulationId_Label_01 for example, with the default pattern it will display img-1-1.png when first receiving the stimulation, then img-1-2.png when receiving it a secont time, etc. After presenting all of the images in a class, it will loop back to image 1.

You can set the number of images by class in the parameters, and add new classes by adding parameters. You must have the same number of images in each class (but of course you don't have to actually use all of them in an experiment).

There is no maximum number of images or classes, but be aware that all images will be preloaded at initialization time, so using a big number of high resolution images will use much memory.
