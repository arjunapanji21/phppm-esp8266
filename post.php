<?php

$servername = "localhost";

// REPLACE with your Database name
$dbname = "id18388506_monitoring";
// REPLACE with Database user
$username = "id18388506_root";
// REPLACE with Database user password
$password = "y*s1SwDJRwRAB0zI";

$conn = mysqli_connect($servername, $username, $password, $dbname);

$ph= $ppm = "";

ini_set('date.timezone', 'Asia/Jakarta');

$now = new DateTime();
$datenow = $now->format("Y-m-d H:i:s");
$ph = $_GET['ph'];
$ppm = $_GET['ppm'];

$sql = "INSERT INTO monitoring(ph, ppm, waktu) VALUES ('$ph', '$ppm', '$datenow')";

if ($conn->query($sql) === TRUE) {
	echo json_encode("Ok");
} else {
	echo "Error: " . $sql . "<br>" . $conn->error;
}



$conn->close();
?>
