#!/bin/bash

#SBATCH --account=tra22_IngInfBo
#SBATCH --partition=g100_usr_bmem
#SBATCH -t 00:05:00
#SBATCH --nodes=1 # due nodi diversi non condividono memoria
#SBATCH --ntasks-per-node=1
#SBATCH -c 48 # CPU cores(OpenMP threads per task) sul singolo nodo
#SBATCH --mem=1000G
#SBATCH -o job.out
#SBATCH -e job.err

# execution line
srun esame_OMP "$1" "$2"
