<head>
  <style>
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
  </style>
</head>

<body>
<?php

$db = new SQLite3("/home/white/prog/foo.db");
if(isset($_POST['sessionset'])){
  $sess = intval($_POST['sessionset']);
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
    $st->bindValue(':sess',$sess,SQLITE3_INTEGER);
    $st->bindValue(':val',$a,SQLITE3_TEXT);
    $st->execute();
  }
  foreach($axis2 as $a){
    $st = $db->prepare("insert into axes(session,value,axis) values(:sess,:val,2)");
    $st->bindValue(':sess',$sess,SQLITE3_INTEGER);
    $st->bindValue(':val',$a,SQLITE3_TEXT);
    $st->execute();
  }
} else {
  $sess = intval($_GET["session"]);
  if(isset($_POST['axis1'])){
    $axis1 = $_POST['axis1'];
    $axis2 = $_POST['axis2'];
    $st = $db->prepare('insert into done(session,value1,value2) values(:sess,:v1,:v2)');
    $st->bindValue(':sess',$sess,SQLITE3_TEXT);
    $st->bindValue(':v1',$axis1,SQLITE3_TEXT);
    $st->bindValue(':v2',$axis2,SQLITE3_TEXT);
    $st->execute();
  } 
  
  $axis1vals = array();
  $axis2vals = array();
  
  $st = $db->prepare("select * from axes where axis=:ax and session=:sess");
  $st->bindValue(':ax',1);
  $st->bindValue(':sess',$sess);
  $r=$st->execute();
  print "<h1>Session $sess</h1>";
  while($row = $r->fetchArray()){
    $axis1vals[] = $row['value'];
  }
  $st->reset();
  $st->bindValue(':ax',2);
  $st->bindValue(':sess',$sess);
  $r=$st->execute();
  while($row = $r->fetchArray()){
    $axis2vals[] = $row['value'];
  }
  print "<table>";
  # output col names
  print "<tr><th/>";
  foreach($axis2vals as $a2){
    print "<th>$a2</th>";
  }
  print "</tr>\n";
  $st = $db->prepare("select * from done where session=:sess and value1=:v1 and value2=:v2");
  $st->bindValue(':sess',$sess,SQLITE3_INTEGER);
  foreach($axis1vals as $a1){
    print "<tr><th>$a1</th>";
    foreach($axis2vals as $a2){
      $st->bindValue(':v1',$a1,SQLITE3_TEXT);
      $st->bindValue(':v2',$a2,SQLITE3_TEXT);
      $r = $st->execute();
      $filled = ($row = $r->fetchArray());
      $class = $filled ? "done" : "notdone";
      print "<td class=\"$class\"/>";
      $st->reset();
    }
    print "</tr>\n";
  }
  print "</table>";
}


?>
</body>

