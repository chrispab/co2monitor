<?php

// $servername = "127.0.0.1";
$servername = "mysql";
// REPLACE with your Database name
$dbname = "co2_monitor";
// REPLACE with Database user
$username = "co2sensor";
// REPLACE with Database user password
$password = "co2sensorpassword";

// Create connection
$conn = new mysqli($servername, $username, $password, $dbname);
// Check connection
if ($conn->connect_error) {
    die("Connection failed: " . $conn->connect_error);
} 

$sql = "SELECT id, co2, temp, humidity, sample_time FROM readings order by sample_time desc";

$result = $conn->query($sql);

while ($data = $result->fetch_assoc()){
    $sensor_data[] = $data;
}

$readings_time = array_column($sensor_data, 'sample_time');

// ******* Uncomment to convert readings time array to your timezone ********
/*$i = 0;
foreach ($readings_time as $reading){
    // Uncomment to set timezone to - 1 hour (you can change 1 to any number)
    $readings_time[$i] = date("Y-m-d H:i:s", strtotime("$reading - 1 hours"));
    // Uncomment to set timezone to + 4 hours (you can change 4 to any number)
    //$readings_time[$i] = date("Y-m-d H:i:s", strtotime("$reading + 4 hours"));
    $i += 1;
}*/
// const co2series = [];
// $sensor_data.    .forEach ($sensor_data[]){
//   $co2series;

// };


$co2Values = json_encode(array_reverse(array_column($sensor_data, 'co2')), JSON_NUMERIC_CHECK);
$value2 = json_encode(array_reverse(array_column($sensor_data, 'temp')), JSON_NUMERIC_CHECK);
$value3 = json_encode(array_reverse(array_column($sensor_data, 'humidity')), JSON_NUMERIC_CHECK);
$reading_time = json_encode(array_reverse($readings_time), JSON_NUMERIC_CHECK);



/*echo $value1;
echo $value2;
echo $value3;
echo $reading_time;*/


// try data in this format

// data: [
//   [datetime, co2],
//   [1, 2],
//   [2, 8]
// ]
// x,y

// g https://jsfiddle.net/BlackLabel/5wsL6euf/

// conmverrt $this->Array ( [0] => Array ( [id] => 707 [co2] => 713 [temp] => 23.00 [humidity] => 92.30 [sample_time] => 2021-09-09 23:58:55 ) [1] => Array ( [id] => 706 [co2] => 715 [temp] => 23.00 [humidity] => 92.20 [sample_time] => 2021-09-09 23:58:40 ) [2] => Array ( [id] => 705 [co2] => 716 [temp] => 23.10 [humidity] => 91.90 [sample_time] => 2021-09-09 23:58:25 ) [3] => Array ( [id] => 704 [co2] => 720 [temp] => 23.00 [humidity] => 92.40 [sample_time] => 2021-09-09 23:58:10 ) [4] => Array ( [id] => 703 [co2] => 725 [temp] => 23.10 [humidity] => 92.20 [sample_time] => 2021-09-09 23:57:55 ) [5] => Array ( [id] => 702 [co2] => 729 [temp] => 23.10 [humidity] => 92.40 [sample_time] => 2021-09-09 23:57:40 ) [6] => Array ( [id] => 701 [co2] => 733 [temp] => 23.10 [humidity] => 92.00 [sample_time] => 2021-09-09 23:57:25 ) [
  // to $this->;;
//   Array
// (
//     [0] => Array
//         (
//             [id] => 778
//             [co2] => 734
//             [temp] => 23.00
//             [humidity] => 92.70
//             [sample_time] => 2021-09-10 00:16:48
//         )

//     [1] => Array
//         (
//             [id] => 777
//             [co2] => 734
//             [temp] => 23.00
//             [humidity] => 92.90
//             [sample_time] => 2021-09-10 00:16:33
//         )

  // https://jsfiddle.net/gh/get/library/pure/highcharts/highcharts/tree/master/samples/highcharts/series/data-array-of-arrays-datetime/
//   Highcharts.chart('container', {
//     xAxis: {
//         type: 'datetime'
//     },

//     series: [{
//         data: [
//             [Date.UTC(2010, 0, 1), 29.9],
//             [Date.UTC(2010, 2, 1), 71.5],
//             [Date.UTC(2010, 3, 1), 106.4]
//         ]
//     }]
// });

$result->free();
$conn->close();
?>

<!DOCTYPE html>
<html>

<head>
  <meta charset="utf-8">
  <title>CO2 Monitor</title>
  <!-- <link rel="shortcut icon" type="image/x-icon" href="/favicon.ico"/> -->
  <link rel="preconnect" href="https://fonts.googleapis.com">
  <link rel="preconnect" href="https://fonts.gstatic.com" crossorigin>
  <link href="https://fonts.googleapis.com/css2?family=Titillium+Web:wght@200&display=swap" rel="stylesheet">

  <meta name="viewport" content="width=device-width, initial-scale=1" />
  <script src="https://code.highcharts.com/highcharts.js"></script>
  <script src="//cdn.rawgit.com/Mikhus/canvas-gauges/gh-pages/download/2.1.7/radial/gauge.min.js"></script>
  <style>
    * {
      box-sizing: border-box;
      font-family: 'Titillium Web', sans-serif;
    }

    body {
      margin: 25px;
      background-color: rgb(127, 156, 126);
    }

    /* Style the header */
    .header {
      /* background-color: #f1f1f1; */
      padding: 10px;
      text-align: center;
    }

    /* Style the top navigation bar */
    .topnav {
      overflow: hidden;
      background-color: #333;
    }

    /* Style the topnav links */
    .topnav a {
      float: left;
      display: block;
      color: #f2f2f2;
      text-align: center;
      padding: 14px 16px;
      text-decoration: none;
    }

    /* Change color on hover */
    .topnav a:hover {
      background-color: #ddd;
      color: black;
    }

    /* Create three unequal columns that floats next to each other */
    .column {
      float: left;
      padding: 10px;
      background-color: rgb(177, 107, 107);
      margin: 10px;

    }

    /* Left and right column */
    .column.left-side {
      width: 275px;
    }

    /* Middle column */
    .column.middle {
      width: 63%;
    }

    .chart {
      width: auto;
      height: 400px;
    }

    .column.right-side {
      width: 275px;
      /* height: 100%; */
    }

    .row {
      border: black;
      border-style: solid;
      background-color: rgb(109, 68, 68);
      border-radius: 10px;
    }

    /* Clear floats after the columns */
    .row:after {
      content: "";
      display: table;
      clear: both;
    }

    meter {
      width: 200px;
      height: 50px;
      border: 1px solid rgb(29, 27, 27);
      border-radius: 3px;

      padding-left: 0;
      padding-right: 0;
      margin-left: auto;
      margin-right: auto;
      display: block;
      /* width: 800px; */
    }

    /* Responsive layout - makes the three columns stack on top of each other instead of next to each other */
    @media screen and (max-width: 600px) {

      .column.left-side,
      .column.right-side,

      .column.middle {
        width: 100%;
      }
    }
  </style>
</head>

<body>

  <div class="header">
    <h1>CO<sub>2</sub> Monitor</h1>
  </div>

  <div class="row">
    <div class="column left-side">
      <h2>Sensor</h2>
      <canvas id="co2-gauge" data-type="radial-gauge" data-width="250" data-height="250" data-units="CO2 ppm"
        data-title="false" data-value="400" data-min-value="400" data-max-value="1800"
        data-major-ticks="400,600,800,1000,1200,1400,1600,1800" data-minor-ticks="2" data-stroke-ticks="false"
        data-value-int="1" data-value-dec="0" data-font-value="courier" data-highlights='[
      { "from": 400, "to": 700, "color": "rgba(0,255,0,.5)" },
      { "from": 700, "to": 1400, "color": "rgba(255,255,0,.5)" },
      { "from": 1400, "to": 1800, "color": "rgba(255,0,0,.5)" }
  
  ]' data-color-plate="#222" data-color-major-ticks="#f5f5f5" data-color-minor-ticks="#ddd" data-color-title="#fff"
        data-color-units="#ccc" data-color-numbers="#eee" data-color-needle-start="rgba(240, 128, 128, 1)"
        data-color-needle-end="rgba(255, 160, 122, .9)" data-value-box="true" data-animation-rule="bounce"
        data-animation-duration="500" data-animated-value="true"></canvas>
    </div>

    <div class="column middle">
      <!-- <h2>middle</h2> -->
      <div id="chart-co2" class="chart"></div>
    </div>

    <div class="column right-side">
      <h2>Readings</h2>
      <h3 id="co2_level">CO<sub>2</sub> level(ppm): </h3>
      <h4 id="temperature_level">Temperature (C): </h4>
      <h4 id="humidity_level">Humidity (%): </h4>
      <h4 id="time">Time : </h4>
      <div>
        <meter class="co2_meter" id="meter_value" min="400" low="700" high="1400" max="1800" optimum="500"
          value="400"></meter>
      </div>
    </div>

  </div>
  <!-- <?php echo var_dump($sensor_data); ?>; -->
  <?php echo '<pre>'; ?>;
  <?php echo print_r($sensor_data); ?>;
  <?php echo $co2Values; ?>;<?php echo $reading_time; ?>;
</body>
<script>








  var co2Values = <?php echo $co2Values; ?>;
  var reading_time = <?php echo $reading_time; ?>;

  var chartT = new Highcharts.Chart({
    chart: { renderTo: "chart-co2", type: 'spline', zoomType: 'x' },
    title: { text: "CO2 Level - ppm" },
    series: [
      {
        showInLegend: false,
        data: co2Values,
        name: 'co2 ppm'
      },
    ],
    plotOptions: {
      spline: {
        animation: true,
        dataLabels: { enabled: false },
      },
      series: { color: "#000000" },
      
    },
    xAxis: {
      type: "datetime",
      labels: {
            format: '{value:%k-%M-%S, reading_time}',
            rotation: 45,
            align: 'left'
        },
      // dateTimeLabelFormats: { second: "%H:%M:%S" },
      // categories: reading_time,

    },
    yAxis: {
      title: { text: "CO2 - ppm" },
      plotBands: [
        {
          color: "#9cc940", // Color value
          from: 300, // Start of the plot band
          to: 700, // End of the plot band
        },
        {
          color: "#f0cf56", // Color value
          from: 700, // Start of the plot band
          to: 1400, // End of the plot band
        },
        {
          color: "#d13d44", // Color value
          from: 1400, // Start of the plot band
          to: 1800, // End of the plot band
        },
      ],
    },

    credits: { enabled: false },
  });
  Highcharts.setOptions({
    global: { useUTC: false }
  });

  // setInterval(function () {
  //   var xhttp = new XMLHttpRequest();
  //   xhttp.onreadystatechange = function () {
  //     if (this.readyState == 4 && this.status == 200) {
  //       var x = new Date().getTime(),
  //         y = parseInt(this.responseText);
  //       console.log(this.responseText);
  //       if (chartT.series[0].data.length > 2160) {
  //         chartT.series[0].addPoint([x, y], true, true, true);
  //       } else {
  //         chartT.series[0].addPoint([x, y], true, false, true);
  //       }
  //       // var s = document.getElementById(meter_value).value;
  //       // s.value = y;
  //       document.getElementById("meter_value").value = y;
  //       // document.getElementById("co2_level").innerHTML = y;

  //       let str = document.getElementById("co2_level").innerHTML
  //       encharloc = str.lastIndexOf(":")
  //       str.substring(0, encharloc)
  //       document.getElementById("co2_level").innerHTML = str.substring(0, encharloc + 2) + y

  //       document.getElementById("co2_level").innerHTML = String(document.getElementById("co2_level").innerHTML).substring(0, String(document.getElementById("co2_level").innerHTML).lastIndexOf(":") + 2) + y;



  //       var gaugeElement = document.getElementsByTagName("canvas")[0];

  //       gaugeElement.setAttribute("data-value", y);
  //       var gauge = document.gauges.get("co2-gauge");
  //       gauge.update();
  //     }
  //   };
  //   xhttp.open("GET", "/co2", true);
  //   xhttp.send();
  //   // every 15 secs
  // }, 15000);

  //! fetch temperature periodically
  setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        var x = new Date().getTime(),
          y = parseFloat(this.responseText);
        console.log(this.responseText);

        // document.getElementById("temperature_level").innerHTML = y;
        document.getElementById("temperature_level").innerHTML = String(document.getElementById("temperature_level").innerHTML).substring(0, String(document.getElementById("temperature_level").innerHTML).lastIndexOf(":") + 2) + y;

      }
    };
    xhttp.open("GET", "/temperature", true);
    xhttp.send();
  }, 30000); // 30 secs

  //! fetch humidity_level periodically
  setInterval(function () {
    var xhttp = new XMLHttpRequest();
    xhttp.onreadystatechange = function () {
      if (this.readyState == 4 && this.status == 200) {
        var x = new Date().getTime(),
          y = parseFloat(this.responseText);
        console.log(this.responseText);
        // document.getElementById("humidity_level").innerHTML = y;
        document.getElementById("humidity_level").innerHTML = String(document.getElementById("humidity_level").innerHTML).substring(0, String(document.getElementById("humidity_level").innerHTML).lastIndexOf(":") + 2) + y;

      }
    };
    xhttp.open("GET", "/humidity", true);
    xhttp.send();
  }, 30000); // 30 secs


  //! fetch time periodically
  setInterval(function () {

    var x = new Date().getTime();
    // var datestringx = x.getHours() + ":" + x.getMinutes() + ":" + x.getSeconds();

    var d = new Date();

    var datestringd = d.getDate() + "-" + (d.getMonth() + 1) + "-" + d.getFullYear() + " " +
      d.getHours() + ":" + d.getMinutes() + ":" + d.getSeconds();

    // console.log(x);
    // console.log(d);

    // document.getElementById("time").innerHTML = datestringd;
    // document.getElementById("time").innerHTML = document.getElementById("time").innerHTML.substring(0, document.getElementById("time").innerHTML.lastIndexOf(":") +2) + datestringd;
    document.getElementById("time").innerHTML = String(document.getElementById("time").innerHTML).substring(0, String(document.getElementById("time").innerHTML).indexOf(":") + 2) + datestringd;

  }, 1000);
</script>

</html>