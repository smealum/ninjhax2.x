<?

error_reporting(0);

function getRegion($v)
{
	return $v[4];
}

function getFirmVersion($v)
{
	if($v[5]=="NEW")
	{
		return "N3DS";
	}else{
		if($v[0]<5)
		{
			return "PRE5";
		}else{
			return "POST5";
		}
	}
}

function getMenuVersion($v)
{
	if ($v[1]==0 or $v[1]==1)
	{
		return "11272";
	}
	else if ($v[1]==2)
	{
		return "12288";
	}
	else if (($v[1]==3 or $v[1]==4))
	{
		return "13330";
	}
	else if ($v[1]==5)
	{
		return "15360";
	}
	else if ($v[1]==6)
	{
		return "16404";
	}
	else if ($v[1]==7)
	{
		return "17415";
	}
	else if ($v[1]==9 and $v[4]=="U")
	{
		return "20480_usa";
	}
	else if ($v[1]>=8)
	{
		return "19456";
	}
}


function getMsetVersion($v)
{
	if($v[0] == 9 and $v[1] < 6)
	{
		return "8203";
	}
	else
	{
		return "9221";
	}
}

$version = array(
		0 => $_GET['zero'],
		1 => $_GET['one'],
		2 => $_GET['two'],
		3 => $_GET['three'],
		4 => $_GET['four'],
		5 => $_GET['five']
	);

$filename="./unsupported.png";

// check that version is valid-ish
if(is_numeric($version[0]) && is_numeric($version[1]) && is_numeric($version[2]) && is_numeric($version[3]))
{
	$filename="./q/".getFirmVersion($version)."_".getRegion($version)."_".getMenuVersion($version)."_".getMsetVersion($version).".png";
}

if(!file_exists($filename))
{
	$filename="./unsupported.png";
}

$fp = fopen($filename, 'rb');

// // send the right headers
header("Content-Type: image/png");
header("Content-Length: " . filesize($filename));

// dump the picture and stop the script
fpassthru($fp);

exit;

?>
