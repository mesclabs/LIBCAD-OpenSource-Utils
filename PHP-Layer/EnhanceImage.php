<?php
ini_set('MAX_EXECUTION_TIME', -1);
require_once("upload.php");

/* EnhanceImage This service enhances DICOM Mammography images .
   % ------------------------------------------------------------------------------------------------

   % Input:-
   %  image              : Binary DICOM image with ID "image".
   %  enhancementFactor  : Integer represents enhancement tuning factor. usually around 10.

   Output
   %  $output            : Encoded JSON string consists of the following keys :
   %  imagePath          : The Full Path of the output image.
   %  imageWidth         : The output image width.
   %  imageHeight        : The output image height.
   % -----------------------------------------------------------------------------------------------*/

//uploading image
$is_uploaded = store_uploaded_file();

if(!$is_uploaded || !isset($_POST['enhancementFactor'])) {
    return;
}
else {
    $image_path = $is_uploaded;
    $enhancment_facor = $_POST['enhancementFactor'];
    exec("cad-cs.exe enhance $image_path $enhancment_facor",$output);
    echo $output[0];
}

?>
