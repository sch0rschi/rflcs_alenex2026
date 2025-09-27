#!/bin/bash

### Ask user first with 60-second timeout
echo "Choose experiment mode (default: All after 60 seconds):"
echo "1) All (>120h)"
echo "2) Solvable (<1h)"

timeout=60
read -t $timeout -p "Enter your choice (1 or 2): " user_input

if [ -z "$user_input" ]; then
    echo -e "\nNo input detected. Defaulting to ALL instance complexity categories."
    choice="all"
else
    case $user_input in
        1)
            echo "You selected ALL instance complexity categories."
            choice="all"
            ;;
        2)
            echo "You selected SOLVABLE instance complexity categories."
            choice="solvable"
            ;;
        *)
            echo "Invalid choice. Defaulting to ALL instance complexity categories."
            choice="all"
            ;;
    esac
fi

### Build
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
cp build/rflcs ./rflcs

### Run experiments
if [ "$choice" = "all" ]; then
    echo "Running ALL instance complexity categories..."
    find ./RFLCS_instances/**/* -type f -exec ./rflcs -i {} \;
else
    echo "Running SOLVABLE instance complexity categories..."
    find ./RFLCS_instances/type0/{32,64,128,256,512}* -type f -exec ./rflcs -i {} \;
    find ./RFLCS_instances/type0/{1024,2048,4096}* -type f ! -name '*_n-div-8.*' -exec ./rflcs -i {} \;
    find ./RFLCS_instances/type1/* -type f -exec ./rflcs -i {} \;
fi

### Create result artefacts
python3 -m venv .venv
source .venv/bin/activate
python3 -m pip install pandas seaborn matplotlib
cd results || exit
python3 latex_table.py
python3 box_plots.py
deactivate
pdflatex -jobname=results main.tex
mv results.pdf ../results.pdf
rm -rf __pycache__
rm ./*.aux
rm ./*.log
rm ./*.pdf
rm ./table_*.tex
cd ..
