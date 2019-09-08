<?php
/**
 * @file
 * Importación de empleados desde API externa a estructura Drupal
 *
 * @author Alberto Chaves (mackpipe@gmail.com, https://github.com/mackpipe).
 */

/**
 * @file
 * Form Configuration module employes rest Api
 */
 
function employes_rest_api_admin_form_basic() {

	$form['employes_rest_api_url'] = array(
		'#type' => 'textfield',
		'#title' => t('Url Employes Rest API'),
		'#default_value' => variable_get('employes_rest_api_url', "http://dummy.restapiexample.com/api/v1/employees"),
		'#description' => t('Set url Employes Rest API'),
		'#required' => TRUE,
	  );	
	 
	return system_settings_form($form);
}