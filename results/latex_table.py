from aggregate import type0_aggregated_df, type1_aggregated_df
from formatting import format_opt, clamp_format_min, format_max, float_format, format_min, bold_30


def create_table(df,
                 filename,
                 caption,
                 label,
                 heading_property_1,
                 heading_property_2):
    table_prefix = ('''\\begin{table}[!ht]
    \\centering
    \\caption{<caption>}
    \\label{<label>}
    \\resizebox{\\textwidth}{!}{
    \\begin{tabular}{c|c|c|cccccc|cccc|cccccccc}
    \\hline
    \\multirow{2}{*}{<heading_property_1>} & \\multirow{2}{*}{<heading_property_2>} & \\multirow{2}{*}{obj best}
    & \\multicolumn{6}{c|}{MDD + CPLEX (Horn)}
    & \\multicolumn{4}{c|}{Heuristic}
    & \\multicolumn{7}{c}{MDD + Gurobi} \\\\ \\cline{4-21}
    &  &  & \\multicolumn{1}{r|}{obj} 
    & \\multicolumn{1}{r|}{gap{[}\\%{]}} 
    & \\multicolumn{1}{r|}{$t_{prep}${[}s{]}} 
    & \\multicolumn{1}{r|}{$t_{tot}${[}s{]}} 
    & \\multicolumn{1}{r|}{\\#opt} 
    & red{[}\\%{]} 
    & \\multicolumn{1}{r|}{obj}
    & \\multicolumn{1}{r|}{best}
    & \\multicolumn{1}{r|}{$t_{sol}${[}s{]}} 
    & $t_{tot}${[}s{]} 
    & \\multicolumn{1}{r|}{obj} 
    & \\multicolumn{1}{r|}{gap{[}\\%{]}} 
    & \\multicolumn{1}{r|}{$t_{prep}${[}s{]}} 
    & \\multicolumn{1}{r|}{$t_{tot}${[}s{]}} 
    & \\multicolumn{1}{r|}{\\#opt} 
    & \\multicolumn{1}{r|}{red{[}\\%{]}}
    & \\multicolumn{1}{r|}{$ram_\\text{avg}${[}MB{]}}
    & {$ram_\\text{max}${[}MB{]}} \\\\
    '''
                    .replace("<caption>", caption)
                    .replace("<label>", label)
                    .replace("<heading_property_1>", heading_property_1)
                    .replace("<heading_property_2>", heading_property_2))

    table_postfix = '''\\end{tabular}
        }
    \\end{table}'''

    group_prefix = '''\\multirow{<rows>}{*}{<primary_label>}'''

    group_postfix = ''' \\hline'''

    group = '''& <secondary_label> & <obj_best> & \\multicolumn{1}{r|}{<obj_horn>} & \\multicolumn{1}{r|}{<gap_horn>} & \\multicolumn{1}{r|}{<t_prep_horn>} & \\multicolumn{1}{r|}{<t_tot_horn>} & \\multicolumn{1}{r|}{<opt_horn>} & <red_horn> & \\multicolumn{1}{r|}{<obj_heur>}& \\multicolumn{1}{r|}{<best_heur>} & \\multicolumn{1}{r|}{<t_sol_heur>} & <t_heur> & \\multicolumn{1}{r|}{<obj>} & \\multicolumn{1}{r|}{<gap>} & \\multicolumn{1}{r|}{<t_prep>} & \\multicolumn{1}{r|}{<t_tot>} & \\multicolumn{1}{r|}{<opt>} & \\multicolumn{1}{r|}{<red>} & \\multicolumn{1}{r|}{<memory>} & <memory_max> \\\\'''

    table_data = table_prefix + '\n'

    frac = ''

    for index, row in df.iterrows():
        if frac != row["primary_label"]:
            frac = row["primary_label"]
            table_data += group_postfix + '\n'
            table_data += group_prefix.replace("<rows>", str(df.loc[df["primary_label"] == frac].shape[0])).replace(
                '<primary_label>', str(row["primary_label"])) + '\n'
        table_data += (group
                       .replace('<secondary_label>', str(int(row["secondary_label"])))
                       .replace('<obj_best>',
                                format_max(row['horn_s_best'], [row['horn_s'], row['avg_solution_length']]))
                       .replace('<obj_horn>',
                                format_max(row['horn_s'], [row['horn_s_best'], row['avg_solution_length']]))
                       .replace('<gap_horn>', format_min(row['horn_gap'], [row['avg_gap']]))
                       .replace('<t_prep_horn>', clamp_format_min(row['horn_t_prep'], [row['avg_reduction_runtime']]))
                       .replace('<t_tot_horn>', clamp_format_min(row['horn_t_tot'], [row['avg_solution_runtime']]))
                       .replace('<opt_horn>', format_opt(row['horn_opt_tot'], row['horn_opt_mdd'], row['solved_count'],
                                                         row['reduction_complete_count']))
                       .replace('<red_horn>', format_max(row['horn_reduction'], [row['avg_reduction_quality']]))
                       .replace('<obj_heur>',
                                format_max(row['avg_heuristic_solution_length'], [row['horn_s_best'],
                                                                                  row['avg_solution_length']]))
                       .replace('<best_heur>', bold_30(row["heuristic_best_count"]))
                       .replace('<t_sol_heur>', float_format(row['avg_heuristic_solution_runtime']))
                       .replace('<t_heur>', float_format(row['avg_heuristic_runtime']))
                       .replace('<obj>',
                                format_max(row['avg_solution_length'], [row['horn_s'], row['horn_s_best']]))
                       .replace('<gap>', format_min(row['avg_gap'], [row['horn_gap']]))
                       .replace('<t_prep>', clamp_format_min(row['avg_reduction_runtime'], [row['horn_t_prep']]))
                       .replace('<t_tot>', clamp_format_min(row['avg_solution_runtime'], [row['horn_t_tot']]))
                       .replace('<opt>',
                                format_opt(row['solved_count'], row['reduction_complete_count'], row['horn_opt_tot'],
                                           row['horn_opt_mdd']))
                       .replace('<red>', format_max(row['avg_reduction_quality'], [row['horn_reduction']]))
                       .replace('<memory>', float_format(row['avg_mdd_memory_consumption']))
                       .replace('<memory_max>', float_format(row['max_mdd_memory_consumption']))
                       + '\n')

    table_data += group_postfix + '\n'
    table_data += table_postfix + '\n'

    with open(filename, 'w') as file:
        file.write(table_data)


df = type0_aggregated_df()
create_table(df,
             "table_0.tex",
             "Instance Set 1 Results",
             "tab:results_1",
             "$|\\Sigma|$",
             "$n$"
             )

df = type1_aggregated_df()
create_table(df,
             "table_1.tex",
             "Instance Set 2 Results",
             "tab:results_2",
             "$|\\Sigma|$",
             "reps",
             )
