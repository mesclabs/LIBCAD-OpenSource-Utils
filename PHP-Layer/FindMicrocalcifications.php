<?php
ini_set('MAX_EXECUTION_TIME', -1);
require_once("upload.php");

/* FindMicrocalcifications This service detects microcalcifications in DICOM Mammography images .
   % -----------------------------------------------------------------------------------------------

   % Input:-
   %  image              : Binary DICOM image with ID "image".

   Output
   %  $output            : Encoded JSON string consists of the following keys :
   %  positions          :  Array of objects represents set of detected microcalcifications
   %                              - i: Double represents microcalcification's i posotion on the image.
   %                              - j: Double represents microcalcification's j posoion on the image .

   % ----------------------------------------------------------------------------------------------*/

//uploading image
$is_uploaded = store_uploaded_file();

if(!$is_uploaded) {
    return;
}
else {
    $image_path = $is_uploaded;
    exec("cad-cs.exe dmicro $image_path",$output);
    print $output[0];
}

?>
