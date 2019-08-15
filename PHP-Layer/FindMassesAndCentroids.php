<?php
ini_set('MAX_EXECUTION_TIME', -1);
require_once("upload.php");

/* FindMassesAndCentroids This service detects masses in DICOM Mammography images .
   % -----------------------------------------------------------------------------------------------

   Input:-
   %  image              : Binary DICOM image with ID "image".

   Output
   %  $output            : Encoded JSON string consists of the following keys :
   %  imagePath          : The Full Path of the output image.
   %  imageWidth         : The output image width.
   %  imageHeight        : The output image height.
   %  centroids          : Array of objects represents set of detected masses centroids.
   %                              - i: Double represents centroid's i posotion on the image.
   %                              - j: Double represents centroid's j posoion on the image .
   %                              - p: Double represents the probability of the malegnancy for the centroids.
   % --------------------------------------------------------------------------------------------*/

//uploading image
$is_uploaded = store_uploaded_file();

if(!$is_uploaded) {
    return;
}
else {
    $image_path = $is_uploaded;
    exec("cad-cs.exe dmass $image_path",$output);
    print $output[0];
}

?>
