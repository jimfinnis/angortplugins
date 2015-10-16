<head>
  <meta http-equiv="refresh" content="30"/>
  <style>
  body {
  font-family: sans-serif;
  }
  table,th,td {
  border: 1px solid black;
  border-collapse: collapse;
  }
  th {
    background-color: #a0a0f0;
  }
  .done {
    background-color: #50ff50;
  }
  .notdone {
    background-color: #ff5050;
  }    
  .started {
    background-color: #5050ff;
  }    
  </style>
</head>

<body>
<?php
define("DBUSER","prog");
define("DBPASS","prog");

$db = new PDO("mysql:host=localhost;dbname=prog",DBUSER,DBPASS);
$db->setAttribute(PDO::ATTR_ERRMODE,PDO::ERRMODE_WARNING);

if(isset($_POST['sessionset'])){
  $sess = $_POST['sessionset'];
  print "sess $sess";
  $axis1 = split(":",$_POST['axis1']);
  $axis2 = split(":",$_POST['axis2']);
  
  $st = $db->prepare("delete from axes where session=:sess");
  $st->bindValue(':sess',$sess);
  $st->execute();
  $st = $db->prepare("delete from done where session=:sess");
  $st->bindValue(':sess',$sess);
  $st->execute();
  
  foreach($axis1 as $a){
    $st = $db->prepare("insert into axes(session,value,axis) values(:sess,:val,1)");
    $st->bindValue(':sess',$sess,PDO::PARAM_STR);
    $st->bindValue(':val',$a,PDO::PARAM_STR);
    $st->execute();
  }
  foreach($axis2 as $a){
    $st = $db->prepare("insert into axes(session,value,axis) values(:sess,:val,2)");
    $st->bindValue(':sess',$sess,PDO::PARAM_STR);
    $st->bindValue(':val',$a,PDO::PARAM_STR);
    $st->execute();
  }
  print "DONE";
} else {
  $sess = $_GET["session"];
  if(isset($_POST['axis1'])){
    $sess = $_POST["session"]; // post overrides get. 
    $axis1 = $_POST['axis1'];
    $axis2 = $_POST['axis2'];
    $stat = $_POST['status'];
    print $stat;
    $st = $db->prepare('replace into done(session,value1,value2,status) values(:sess,:v1,:v2,:stat)');
    $st->bindValue(':sess',$sess,PDO::PARAM_STR);
    $st->bindValue(':v1',$axis1,PDO::PARAM_STR);
    $st->bindValue(':v2',$axis2,PDO::PARAM_STR);
    $st->bindValue(':stat',$stat,PDO::PARAM_INT);
    if(!$st->execute()){
      print("FAILED $sess\n");
      print_r($st->errorInfo());
    }
  } 
  
  $axis1vals = array();
  $axis2vals = array();
  
  $st = $db->prepare("select * from axes where axis=:ax and session=:sess");
  $st->bindValue(':ax',1,PDO::PARAM_INT);
  $st->bindValue(':sess',$sess,PDO::PARAM_STR);
  $st->execute();
  print "<h1>Session $sess</h1>";
  $date = date('r');
  print "<h2>$date</h2>";
  while($row = $st->fetchObject()){
    $axis1vals[] = $row->value;
  }
  $st->bindValue(':ax',2);
  $st->bindValue(':sess',$sess);
  $st->execute();
  while($row = $st->fetchObject()){
    $axis2vals[] = $row->value;
  }
  print "<table>";
  # output col names
  print "<tr><th/>";
  foreach($axis2vals as $a2){
    print "<th>$a2</th>";
  }
  print "</tr>\n";
  $st = $db->prepare("select * from done where session=:sess and value1=:v1 and value2=:v2");
  $st->bindValue(':sess',$sess,PDO::PARAM_STR);
  $started=0;
  $done=0;
  foreach($axis1vals as $a1){
    print "<tr><th>$a1</th>";
    foreach($axis2vals as $a2){
      $st->bindValue(':v1',$a1,PDO::PARAM_STR);
      $st->bindValue(':v2',$a2,PDO::PARAM_STR);
      $st->execute();
      $filled = ($row = $st->fetchObject());
      $stat=0;
      if(!$filled)$class="notdone";
      else {
        $stat = intval($row->status);
        if($stat == 1){
          $class="started";
          $started++;
        }else{
          $class="done";
          $done++;
        }
      }
      print "<td class=\"$class\"/>";
    }
    print "</tr>\n";
  }
  print "</table>";
  print "<p>Started: $started, done: $done</p>";
}


?>
</body>

