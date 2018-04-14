<?php

function get_data($url){
    $content = explode("\n", file_get_contents($url));
    $data = array();
    # first row is header
    $head = array();
    $hdrow = array_shift($content);
    foreach(explode(",", $hdrow) as $h){
        $head[] = strtolower(trim($h));
    }

    // parse data
    foreach($content as $row){
        $d = array();
        $els = explode(",", $row);
        if(count($els)!=count($head)) continue;
        foreach($els as $i=>$h){
            $d[$head[$i]] = trim($h);
        }
        $data[] = $d;   
    }

    // sort by timestamp
    usort($d, function($a, $b) {
            return $a['timestamp'] - $b['timestamp'];
    });

    return array_slice($data, -1000);
}


if($_GET['data']==1){
    header('Content-Type: application/json');
    $d = get_data('http://your_server_ip/graph/data.csv');
    echo json_encode($d);
}
else{
    // return the html
?><!doctype html>
<html>
    <head>
        <meta charset="utf-8">
        <meta name="viewport" content="width=device-width, initial-scale=1">
        <title>Radiona.org Weight Scale</title>

        <script type="text/javascript" src="http://code.jquery.com/jquery-1.9.1.min.js"></script>
        <script type="text/javascript" src="http://code.highcharts.com/highcharts.js"></script>
        <script type="text/javascript" src="http://code.highcharts.com/modules/data.js"></script>

        <style type="text/css">
            body{
                font-family: Arial, Helvetica, sans-serif;
                padding:1em;
            }
            #header{
                display:block;
                margin: 1em auto;
                max-width:400px;
                width:100%;
            }
            h1{
                text-align:center;
            }
            table{
                margin:1em auto;
                border-collapse: collapse;
                width:100%;
                max-width:700px;
            }
            table th, table td{
                padding:0.5em;
                border: 1px solid black;
                text-align:left;
                width:50%;
            }
        </style>
        <script type="text/javascript">
            $(function(){
                Highcharts.setOptions({
                  global: {
                      useUTC: false
                            }
                });
                var data = []; 
                var refr = function(){
                    $.get(window.location.href+'?data=1&ts='+(new Date()).getTime(), function(d){
                        var c = $('#container').highcharts();

                        if(c.series[0].data.length==0){
                            // add initial data if empty
                            var dt_new = [];
                            for(var dd in d){
                                dt_new.push([parseInt(d[dd]['timestamp']), parseFloat(d[dd]['weight'])]);
                            }
                            c.series[0].setData(dt_new);
                            return;
                        }

                        // first remove old data
                        while(true){
                            if(c.series[0].data[0].x<parseInt(d[0].timestamp)){
                                c.series[0].data[0].remove();
                                continue;
                            }
                            break;
                        }
                        // then add new data
                        var last_ts = c.series[0].data.length>0 ? c.series[0].data[c.series[0].data.length-1].x:0;
                        var acc = 0;
                        for(var i in d){
                            if(parseInt(d[i].timestamp)>last_ts){
                                // add new point
                                c.series[0].addPoint([parseInt(d[i].timestamp), parseFloat(d[i].weight)]);
                                last_ts = c.series[0].data.length>0 ? c.series[0].data[c.series[0].data.length-1].x:0;
                            }
                            acc += parseFloat(d[i].weight);
                        }

                        $('#count').text(c.series[0].data.length);
                        $('#accum').text((Math.round(acc*100)/100.0)+' kg');
                        $('#avg').text((Math.round(acc/c.series[0].data.length*100)/100.0)+' kg');

                    });
                };
                setInterval(refr, 1000);

                $('#container').highcharts({
                    chart: { type: 'spline' },
                    legend: {enabled:false},
                    series: [ {name: "Mjerenja", marker: {enabled: true, radius: 4}, data: []} ],
                    title: {
                        text: ''
                    },
                    yAxis: {
                        title: {
                            text: 'kg'
                        }
                    },
                    xAxis: [{
                        type: 'datetime',
                        dateTimeLabelFormats: { month: '%e. %b', year: '%Y' },
                        title: { text: 'Date/time' }
                    }],

                });
                refr();
            });
        </script>
    </head>
    <body>
        <img id="header" src="radiona.png" alt="radiona">
        <h1>Radiona vaga v1.0</h1>
        <table>
            <tr><th>Accumulated</th><td id="accum">--</td></tr>
            <tr><th>Average</th><td id="avg">--</td></tr>
            <tr><th>Total</th><td id="count">--</td></tr>
        </table>
        <div id="container" style="width: 800px; height: 400px; margin: 0 auto"></div>
    </body>
</html>
<?php
}
