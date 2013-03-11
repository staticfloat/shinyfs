#!/bin/sh

# Change this to whatever prefix you want to use
cd /tmp

# Little utility to help show the times for a file
function sstat()
{
	stat -f'%B' $1
	stat -f'%a' $1
	stat -f'%c' $1
	stat -f'%m' $1
}

function compare_times()
{
	echo $(stat -f'%B' $1) == $(stat -f'%B' $2)
	echo $(stat -f'%a' $1) == $(stat -f'%a' $2)
	echo $(stat -f'%c' $1) == $(stat -f'%c' $2)
	echo $(stat -f'%m' $1) == $(stat -f'%m' $2)
}

if [[ "$PLATFORM" == "Darwin" ]]; then
	if [[ -z $(mount -t fuse4x | grep $(pwd)/shiny) ]]; then
		echo "ERROR: Must have a shinyfs volume mounted at $(pwd)/shiny"
		exit -1
	fi

	# Cleanup first, in case something has gone wrong before
	echo "Initializing..."
	rm -f time_test shiny/time_test
	rm -rf time_test_dir shiny/time_test_dir	

	# Touch test
	touch time_test shiny/time_test
	sync
	sleep 2
	touch time_test shiny/time_test

	echo "Touch test:"
	compare_times time_test shiny/time_test


	# Write test
	touch time_test shiny/time_test
	sync
	sleep 2
	echo "test" > time_test
	echo "test" > shiny/time_test
	
	echo "Write test:"
	compare_times time_test shiny/time_test
	
	
	# Directory test
	mkdir time_test_dir shiny/time_test_dir
	sync
	sleep 2
	touch time_test_dir/a shiny/time_test_dir/a
	
	echo "Directory test:"
	compare_times time_test_dir shiny/time_test_dir
fi

cd -
