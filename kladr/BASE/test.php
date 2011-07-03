<?php




/*
Код
СС РРР ГГГ ППП АА, где
СС – код субъекта Российской Федерации (региона), коды регионов представлены в Приложении 2 к Описанию классификатора адресов Российской Федерации (КЛАДР);
РРР – код района;
ГГГ – код города;      
ППП – код населенного пункта,
АА – признак актуальности адресного объекта.
*/

// open in read-only mode
$db = dbase_open('./KLADR.DBF', 0);

if ($db) {
  $CLADR = array();
  $NAME_CC = array();
  $NAME_RRR = array();
  $NAME_GGG = array();
  $NAME_PPP = array();
  
  $record_numbers = dbase_numrecords($db);
  for ($i = 1; $i <= $record_numbers; $i++) {
    $row= dbase_get_record($db, $i);
    
    $name=mb_convert_encoding($row[0], "UTF-8", "cp866"); 		// Наименование 
    $socr=mb_convert_encoding($row[1], "UTF-8", "cp866"); 		// Сокращенное наименование типа объекта
    $code=$row[2]; 		// Код
    $index=$row[3]; 	// Почтовый индекс
    $gninmb=$row[4]; 	// Код ИФНС
    $uno=$row[5]; 		// Код территориального участка ИФНС
    $ocatd=$row[6]; 	// Код ОКАТО
    $status=$row[7]; 	// Статус объекта
    
    $CC = (int)substr($code, 0, 2);
    $RRR = (int)substr($code, 2, 3);
    $GGG = (int)substr($code, 5, 3);
    $PPP = (int)substr($code, 8, 3);
    $AA = (int)substr($code, 11, 2);
    
    if ($RRR==0 AND $GGG==0 AND $PPP==0 AND $AA==0){
      $CLADR[$CC]['name'] = $name;
      $CLADR[$CC]['socr'] = $socr;
      $CLADR[$CC]['status'] = $status;
    }
    if ($status>0 AND $AA==0){
      $CLADR[$CC]['child'][] = array('name'=> $name, 'socr'=> $socr, 'status'=>$status); 
    }
  }
  dbase_close($db);
}

$cladr = array();
foreach ($CLADR as $key => $level1){
  $level1_name = $level1['name'];
  $cladr[$level1_name] = array();
  foreach ($level1['child'] as $key2 => $level2){
    $level2_name = $level2['name'];
    array_push($cladr[$level1_name], $level2_name);
  }
}

print_r($cladr);

?>

