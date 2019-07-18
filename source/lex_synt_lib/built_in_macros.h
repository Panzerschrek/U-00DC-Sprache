R"(

?macro <? if_var:block ( ?r:opt<? & ?> ?m:opt<? mut ?> ?var_name:ident : ?e:expr ) ?b:block ?>
->
<?
	{
		auto lock_temps& ?m<? mut ?> ??e_result= ?e;
		if( !??e_result.empty() )
		{
			auto lock_temps ?r<? & ?> ?m<? mut ?> ?var_name= ??e_result.get_ref();
			?b
		}
	}
?>

?macro <? foreach:block ( ?r:opt<? & ?> ?m:opt<? mut ?> ?var_name:ident : ?e:expr ) ?b:block ?>
->
<?
	{
		auto lock_temps& ?m<? mut ?> ??e_result= ?e;
		static_if( typeinfo</ typeof(??e_result) />.is_array )
		{
			var size_type mut ??i(0u);
			while( ??i < typeinfo</ typeof(??e_result) />.element_count )
			{
				auto ?r<? & ?> ?m<? mut ?> ?var_name= ??e_result[??i];
				safe{ ?b }
				++ ??i;
			}
		}
		else
		{
			auto mut ??r= ??e_result.range();
			while( !??r.empty() )
			{
				unsafe
				{
					{
						auto lock_temps ?r<? & ?> ?m<? mut ?> ?var_name= ??r.front_unsafe();
						safe{ ?b }
					}
					??r.pop_front();
				}
			}
		}
	}
?>

)"
