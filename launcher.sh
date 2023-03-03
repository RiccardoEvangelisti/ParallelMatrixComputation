#!/bin/bash

if [[ "$#" -lt 3 ]]; then echo "Usage: sh $0 [--OMP || --MPI] [[--compile]] [--strong DIM || --weak] --nthreads SIZE..." 1>&2; exit 1; fi

OMP=-1
MPI=-1

if [[ "$1" == "--OMP" ]]; then
    echo "--- OMP ---"
    OMP=0
    cd OMP
elif [[ "$1" == "--MPI" ]]; then
    echo "--- MPI ---"
    MPI=0
    cd MPI
else
    echo "Usage: sh $0 [--OMP || --MPI] [[--compile]] [--strong DIM || --weak] --nthreads SIZE..." 1>&2; exit 1
fi

shift 1
echo ""

if [[ "$1" == "--compile" ]]; then
    echo "--- COMPILE ---"
    if [[ $OMP != -1 ]]; then
        gcc -fopenmp esame_OMP.c -o esame_OMP
    elif [[ $MPI != -1 ]]; then
        module load autoload intelmpi && mpiicc -std=c99 esame_MPI.c -o esame_MPI
    fi
    shift 1
fi

##############################################################################

if [[ "$1" == "--strong" ]]; then
    shift 1

    echo ""
    echo "--- STRONG SCALABILITY TEST ---"

    DIM="$1"
    shift 1

    if [[ "$1" != "--nthreads" ]]; then
        echo "Usage: sh $0 [--OMP || --MPI] [[--compile]] [--strong DIM || --weak] --nthreads SIZE..." 1>&2; exit 1
    fi
    shift 1

    strong_scal="strong_scal.txt"
    > $strong_scal

    for i in "$@"
    do
        [ -f "job.out" ] && rm "job.out"
        [ -f "job.err" ] && rm "job.err"

        if [[ $OMP != -1 ]]; then
            # max ntasks-per-node=48
            print=$(sbatch launcher_OMP.sh $DIM $i)
        elif [[ $MPI != -1 ]]; then
            print=$(sbatch --nodes=1 --ntasks-per-node="$i" launcher_MPI.sh $DIM)
        fi

        echo "  $print (dim=$DIM, nthreads=$i)"

        while ! [ -s "job.out" ] && ! [ -s "job.err" ]; do sleep 1; done
        cat "job.out" | awk -F. '{print $1","$2}' >> $strong_scal
        cat "job.err" >> $strong_scal
    done

    [ -f "job.out" ] && rm "job.out"
    [ -f "job.err" ] && rm "job.err"

    echo "Results: "
    cat $strong_scal
    echo ""

##############################################################################

elif [[ "$1" == "--weak" ]]; then
    shift 1

    echo ""
    echo "--- WEAK SCALABILITY TEST ---"

    if [[ "$1" != "--nthreads" ]]; then
        echo "Usage: sh $0 [--OMP || --MPI] [[--compile]] [--strong DIM || --weak] --nthreads SIZE..." 1>&2; exit 1
    fi
    shift 1

    weak_scal="weak_scal.txt"
    > $weak_scal

    for i in "$@"
    do
        [ -f "job.out" ] && rm "job.out"
        [ -f "job.err" ] && rm "job.err"

        DIM=$(echo "scale=4; sqrt($i) * 10000" | bc | awk -F. '{print $1}')

        if [[ $OMP != -1 ]]; then
            # max ntasks-per-node=48
            print=$(sbatch launcher_OMP.sh $DIM $i)
        elif [[ $MPI != -1 ]]; then
            print=$(sbatch --nodes=1 --ntasks-per-node="$i" launcher_MPI.sh $DIM)
        fi

        echo "  $print (dim=$DIM, nthreads=$i)"

        while ! [ -s "job.out" ] && ! [ -s "job.err" ]; do sleep 1; done
        cat "job.out" | awk -F. '{print $1","$2}' >> $weak_scal
        cat "job.err" >> $weak_scal
    done

    [ -f "job.out" ] && rm "job.out"
    [ -f "job.err" ] && rm "job.err"

    echo "Results: "
    cat $weak_scal
    echo ""

else
    echo "Usage: sh $0 [--OMP || --MPI] [[--compile]] [--strong DIM || --weak] --nthreads SIZE..." 1>&2; exit 1
fi