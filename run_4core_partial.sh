#!/bin/bash
if [ $# -lt 1 ] 
then
    echo "Usage : ./run_all_sims.sh <binary> <output>"
    exit
fi

binary=$1
echo $binary
output=$2
echo $output

if [ ! -e "bin/$binary" ] ; then
    echo "Invalid Binary"
    exit 1
fi

for i in `seq 1 30`;
do
#    echo $f $benchmark 
    num=$i

#    output_dir="/scratch/cluster/akanksha/CRCRealOutput/4coreCRC/PAC_AMPM/$output/"
#    output_dir="/scratch/cluster/akanksha/CRCRealOutput/4coreCRC/PAC_BO/$output/"
#    output_dir="/scratch/cluster/akanksha/CRCRealOutput/4coreCRC/PAC_KPC/$output/"
#    output_dir="/scratch/cluster/akanksha/CRCRealOutput/4coreCRC/PAC_LLC/$output/"
#    output_dir="/scratch/cluster/akanksha/CRCRealOutput/4coreCRC/PAC/$output/"
#    output_dir="/scratch/cluster/akanksha/CRCRealOutput/4coreCRC/NP/$output/"
    output_dir="/scratch/cluster/cli23/experiment_current_output/$output"

    if [ ! -e "$output_dir" ] ; then
        mkdir $output_dir
        mkdir "$output_dir/scripts"
    fi
    condor_dir="$output_dir"
    script_name="$num"

    mix_file="$output_dir/mix""$i.txt"
    core0_ipc=`perl get_percore_ipc.pl $mix_file 0`
    core1_ipc=`perl get_percore_ipc.pl $mix_file 1`
    core2_ipc=`perl get_percore_ipc.pl $mix_file 2`
    core3_ipc=`perl get_percore_ipc.pl $mix_file 3`
    #if [ $i -eq 92 ] 
    #then
    if [  1 -eq "$(echo "$core0_ipc == 0.0" | bc)" ] || [  1 -eq "$(echo "$core1_ipc == 0.0" | bc)" ] || [  1 -eq "$(echo "$core2_ipc == 0.0" | bc)" ] || [  1 -eq "$(echo "$core3_ipc == 0.0" | bc)" ]
    then
    echo "Mix "$i
    
                    
#    output_dir="/scratch/cluster/akanksha/CRCRealOutput/4coreCRC/BO/$output/"

    #cd $output_dir
    command="/u/cli23/thesis_research/ChampSim/run_4core.sh $binary 50 150 $num $output_dir"
    #command="/u/akanksha/MyChampSim/ChampSim/scripts/run_4core.sh $binary 200 1000 $num $output_dir -ped_coefficient\ 10"
#    echo $command
#echo "here"

    /u/cli23/thesis_research/condor_shell --silent --log --condor_dir="$condor_dir" --condor_suffix="$num" --output_dir="$output_dir/scripts" --simulate --script_name="$script_name" --cmdline="$command"

        #Submit the condor file
    /lusr/opt/condor/bin/condor_submit $output_dir/scripts/$script_name.condor
    fi
    #fi

done

