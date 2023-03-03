#!/bin/bash

#SBATCH --job-name=evangelisti_MPI
#SBATCH --account=tra22_IngInfBO
#SBATCH --partition=g100_usr_bmem
#SBATCH --time=00:05:00
#SBATCH -o job.out
#SBATCH -e job.err
#SBATCH --mem=1000G

# env. variables and modules
module load autoload intelmpi

# execution line
srun esame_MPI "$1"