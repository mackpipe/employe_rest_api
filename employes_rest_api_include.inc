<?php
/**
 * @file
 * Importación de empleados desde API externa a estructura Drupal
 *
 * @author Alberto Chaves (mackpipe@gmail.com, https://github.com/mackpipe).
 */

/**
 * @file
 * Import employes rest Api
 */
 

/**
 * import employes - sets up the batch
 */
function employes_rest_api_import() {
	$batch = array(
		'operations' => array(),
		'finished' => 'employes_rest_api_batch_finished', // runs after batch is finished
		'title' => t('Processing Import'),
		'init_message' => t('Import is starting.'),
		'progress_message' => t('Processed @current out of @total.'),
		'error_message' => t('Import has encountered an error.'),
		'file' => drupal_get_path('module', 'employes_rest_api') . '/employes_rest_api_include.inc',
	);

	$url=variable_get('employes_rest_api_url');
	if(!empty($url)&&trim($url)!=""){
		
		$response = file_get_contents($url);
		$employes = json_decode($response);
		
		$progress = 0; // where to start
		$limit = 1; // how many to process for each run
		$max = count($employes); // how many records to process until stop - can do query here to get max times to run
		
		foreach ($employes as $key => $employe) {
				$batch['operations'][] = array('employes_rest_api_import_process', array($progress, $limit,$employe));
				$progress = $progress + $limit;
		}	
	}else{
		drupal_set_message(t('The rest API url was not found, please go to admin / config / system / employes_rest_api and save the configuration'),'error');
	}

	batch_set($batch);
	batch_process('admin/content'); // page to return to after complete 
}

/**
 * this is what runs multiple times per batch
 * progress and limit and is updated during each run
 */
function employes_rest_api_import_process($progress, $limit, $employe, &$context) {
  
	$nid=__employes_rest_api_find_employe($employe->id);
	if($nid==0){
		__employes_rest_api_create_node($employe);
			// update progress for message
		  $progress = $progress + $limit; 
		  // update message during each run so you know where you are in the process
		  $context['message'] = 'Now processing employe [' . $employe->id . ' - '.$employe->employee_name.'] Created';
	}else{
		__employes_rest_api_update_node($employe,$nid);
		// update progress for message
		  $progress = $progress + $limit; 
		  // update message during each run so you know where you are in the process
		  $context['message'] = 'Now processing employe [' . $employe->id . ' - '.$employe->employee_name.'] Updated';
	}

	  		
  
}

/**
 * Finished import employes
 */
function employes_rest_api_batch_finished($success, $results, $operations) {
  if ($success) {
    drupal_set_message('Import is complete');
  }
  else {
    $error_operation = reset($operations);
    $message = t('An error occurred while processing %error_operation with arguments: @arguments', array(
      '%error_operation' => $error_operation[0],
      '@arguments' => print_r($error_operation[1], TRUE)
    ));
    drupal_set_message($message, 'error');
  }
}

/**
 * find employes by id remote
 */
function __employes_rest_api_find_employe($remote_id){

	$query = db_select( 'node', 'n' );
	$query->leftJoin('field_data_field_employes_id_remote2', 'er', 'n.nid = er.entity_id');
	$query
	->fields( 'n', array('nid'))	
	->condition( 'n.type', 'empleados' )
	->condition('er.field_employes_id_remote2_value', $remote_id);
	$result = $query->execute();
	$employe_id  = array();
	foreach( $result as $row ) {
		$employe_id[] = $row->nid;
	}
	if(count($employe_id)>0){
		return $employe_id[0];
	}else{
		return 0;
	}	
}

/**
 * Create node with the info of the employe
 */
function __employes_rest_api_create_node($info){
	global $user;

	$node = new stdClass();
	$node->title = "Employe ".$info->id." from API rest";
	$node->type = "empleados";
	// Sets some defaults. Invokes hook_prepare() and hook_node_prepare().
	node_object_prepare($node); 
	
	$node->language = LANGUAGE_NONE; 
	$node->uid = $user->uid; 
	// Status is 1 or 0; published or not.
	$node->status = 1; 
	// Promote is 1 or 0; promoted to front page or not.
	$node->promote = 0; 
	// Comment is 0, 1, 2; 0 = disabled, 1 = read only, or 2 = read/write.
	$node->comment = 1; 

	// Text field
	$node->field_employes_name2[$node->language][]['value'] = $info->employee_name;

	// Text field
	$node->field_employes_age2[$node->language][]['value'] = $info->employee_age;

	// Text field
	$node->field_employes_id_remote2[$node->language][]['value'] = $info->id;

	// Term reference (taxonomy) field
	$node->field_employes_salary_type[$node->language][]['tid'] = __employes_rest_api_get_term_id_salary($info->employee_salary);

	
	$node = node_submit($node); // Prepare node for saving
	node_save($node);

	$t_args = array('%name' =>$info->id);
	watchdog('node', 'Created node for employe %name.', $t_args, WATCHDOG_NOTICE, ''); 
}

/**
 * Update info of the employe by Id Remote
 */
function __employes_rest_api_update_node($info,$nid){
	global $user;
		
	$node = node_load ($nid);
	
	// Text field
	$node->field_employes_name2[$node->language][]['value'] = $info->employee_name;

	// Text field
	$node->field_employes_age2[$node->language][]['value'] = $info->employee_age;

	// Text field
	$node->field_employes_id_remote2[$node->language][]['value'] = $info->id;

	// Term reference (taxonomy) field
	$node->field_employes_salary_type[$node->language][]['tid'] = __employes_rest_api_get_term_id_salary($info->employee_salary);
	
	node_save($node);

	$t_args = array('%name' =>$info->id);
	watchdog('node', 'Updated node for employe %name.', $t_args, WATCHDOG_NOTICE, '');
}

/**
 * get salary type of employe
 */
function __employes_rest_api_get_term_id_salary($salary) {
	$terms_taxonomy=array('Salario bajo','Salario medio','Salario alto');
	$term_name="";
	if($salary>=0&& $salary<=1000){
		$term_name=$terms_taxonomy[0];
	}
	if($salary>1000&& $salary<=4000){
		$term_name=$terms_taxonomy[1];
	}
	if($salary > 4000){
		$term_name=$terms_taxonomy[2];
	}

	$term_id=__employes_rest_api_get_term_from_name(trim($term_name),'tipo_salario');
	return $term_id;
}

/**
 * Get term id for taxonomy term by name
 */
function __employes_rest_api_get_term_from_name($term_name, $vocabulary_name) {
  if ($vocabulary = taxonomy_vocabulary_machine_name_load($vocabulary_name)) {
    $tree = taxonomy_get_tree($vocabulary->vid);
    foreach ($tree as $term) {
      if ($term->name == $term_name) {
        return $term->tid;
      }
    }
  }

  return false;
}