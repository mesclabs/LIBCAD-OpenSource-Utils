<?php

function store_uploaded_file() {
    //validating registers

    if(!isset($_FILES['image']["name"])) {
        echo "invalid image";
        return false;
    }
    else {
        $target_dir = 'inputs';
        $today_date = date("Y-m-d");
        $token = uniqid(mt_rand(), true);

        //creating inputs directory
        if (!file_exists($target_dir)) {
            mkdir($target_dir, 0777, true);
        }

        $today_dir = $target_dir.trim('\ ',' ').$today_date;

        if (!file_exists($today_dir)) {
            mkdir($today_dir, 0777, true);
        }

        $target_file = $today_dir . trim('\ ',' ').$token . basename($_FILES['image']["name"]);

        // Check if file already exists
        if (file_exists($target_file)) {
            while(true) {
                $token = uniqid(mt_rand(), true);

                $target_file = $today_dir . trim('\ ',' ').$token . basename($_FILES['image']["name"]);

                if(!file_exists($target_file)) break;
            }
        }

        if (move_uploaded_file($_FILES['image']["tmp_name"], $target_file)) {
            $full_img_path = getcwd().trim('\ ',' '). $target_file;
            return $full_img_path;
        } else {
            echo "Sorry, there was an error uploading your file.";
            return false;
        }
    }
}

?>
