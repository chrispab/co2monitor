<?php
/*
  Rui Santos
  Complete project details at https://RandomNerdTutorials.com

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files.

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.
*/

$servername = "mysql";

// REPLACE with your Database name
$dbname = "co2_monitor";
// REPLACE with Database user
$username = "co2sensor";
// REPLACE with Database user password
$password = "co2sensorpassword";

// Keep this API Key value to be compatible with the ESP32 code provided in the project page. If you change this value, the ESP32 sketch needs to match
$api_key_value = "tPmAT5Ab3j7F9";

$api_key = $value1 = $value2 = $value3 = "";  // init all values at start of call to this script

echo "reqest made";
// console.log("Message here");

echo "hello world!" . PHP_EOL;


if ($_SERVER["REQUEST_METHOD"] == "POST") {
    echo "reqest made 2";
    $api_key = clean_input($_POST["api_key"]);//get the incoming key from the request
    if ($api_key == $api_key_value) {
        $value1 = clean_input($_POST["value1"]);
        $value2 = clean_input($_POST["value2"]);
        $value3 = clean_input($_POST["value3"]);
        
        // Create connection
        $conn = new mysqli($servername, $username, $password, $dbname);
        // Check connection
        if ($conn->connect_error) {
            die("Connection failed: " . $conn->connect_error);
        }
        
        $sql = "INSERT INTO readings (co2, temp, humidity)
        VALUES ('" . $value1 . "', '" . $value2 . "', '" . $value3 . "')";
        
        if ($conn->query($sql) === true) {
            echo "New record created successfully";
        } else {
            echo "Error: " . $sql . "<br>" . $conn->error;
        }
    
        $conn->close();
    } else {
        echo "Wrong API Key provided.";
    }
} else {
    echo "No data posted with HTTP POST.";
}

function clean_input($data)
{
    $data = trim($data);
    $data = stripslashes($data);
    $data = htmlspecialchars($data);
    return $data;
}
