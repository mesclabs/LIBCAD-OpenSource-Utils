<?php
ini_set('MAX_EXECUTION_TIME', -1);
require_once("upload.php");

/* EnhanceImage This service enhances partial view of  DICOM Mammography images .
   % -----------------------------------------------------------------------------------------------

   % Input:-
   %  image              : Binary DICOM image with ID "image".
   %  enhancementFactor  : Integer represents enhancement tuning factor. usually around 10.
   %  i                  : number represents the horizontal coordinate of the point in the center of the window which will be enhanced .
   %  j                  : number represents the vertical coordinate of the point in the center of the window which will be enhanced .
   %  level              : double represents the width of the square partial view to be enhanced .

   Output
   %  $output[0]         : JSON encoded string consists of the following keys :

   %  positions          : An array that holds the indicies of all microcalcifications.
   % ----------------------------------------------------------------------------------------------*/

//uploading image
$is_uploaded = store_uploaded_file();

if(!$is_uploaded ||  !isset($_POST['enhancementFactor']) || !isset($_POST['i']) || !isset($_POST['j'])) {
    return;
}
else {
    $image_path = $is_uploaded;
    $enhancemrnt_factor = $_POST['enhancementFactor'];
    $i = $_POST['i'];
    $j = $_POST['j'];
    $level = $_POST['level'];
    exec("cad-cs.exe lenhance $image_path $enhancemrnt_factor $i $j $level",$output);
    echo $output[0];
}

?>
