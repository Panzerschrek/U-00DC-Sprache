R"(
?macro <? if_var:block ( ?r:opt<? & ?> ?m:opt<? mut ?> ?var_name:ident : ?e:expr ) ?b:block ?>
->
<?
	{
		auto lock_temps& ?m<? mut ?> e_result= ?e;
		if( !e_result.empty() )
		{
			auto lock_temps ?r<? & ?> ?m<? mut ?> ?var_name= e_result.get_ref();
			?b
		}
	}
?>
)"
