from aggregate import type0_df, type1_df
import seaborn as sns
import matplotlib.pyplot as plt
import re


def div_fraction_format(category):
    pattern = r'(\d*)n-div-(\d+)'
    match = re.match(pattern, category)
    if match:
        top = match.group(1)
        bottom = match.group(2)
        if top == "":
            return "$\Sigma = \\frac{n}{" + str(bottom) + "}$"
        else:
            return "$\Sigma = \\frac{" + str(top) + "\\cdot n}{" + str(bottom) + "}$"


def repetitions_format(category):
    return category + " repetitions"

def box_plot(df, title, x_label, y_label, file_template, projection, categories, selector_property, running_property,
             plot_label_format):
    selected_df = df[projection]

    for category in categories:
        subset_rows = selected_df[selected_df[selector_property] == category].reset_index(drop=True)
        subset_rows = subset_rows.drop(columns=[selector_property])

        df_melted = subset_rows.melt(id_vars=[running_property], var_name='Statistic', value_name='Value')

        plt.figure()
        sns.boxplot(x=running_property, y='Value', data=df_melted, whis=[0, 100], showmeans=True, meanprops={"marker":"D", "markerfacecolor":"black", "markeredgecolor":"black"})

        plt.title(title + plot_label_format(category))
        plt.xlabel(x_label)
        plt.ylabel(y_label)
        plt.xticks(rotation=45)

        plt.grid(True)
        plt.savefig(file_template.replace("<>", category) , format="pdf", bbox_inches="tight")
        plt.close()

df0 = type0_df()
box_plot(df0,
         "Runtime Statistics for ",
         "$n$",
         "Runtime [s]",
         "runtime_type0_<>.pdf",
         ["Property_2", "Property_1", 'solution_runtime'],
         ["n-div-8", "n-div-4", "3n-div-8", "n-div-2", "5n-div-8", "3n-div-4", "7n-div-8"],
         "Property_2",
         "Property_1",
         div_fraction_format
         )

box_plot(df0,
         "Solution Length Statistics for ",
         "$n$",
         "RFLCS length",
         "solution_type0_<>.pdf",
         ["Property_2", "Property_1", 'solution_length'],
         ["n-div-8", "n-div-4", "3n-div-8", "n-div-2", "5n-div-8", "3n-div-4", "7n-div-8"],
         "Property_2",
         "Property_1",
         div_fraction_format
         )

box_plot(df0,
         "MDD Memory Usage Statistics for ",
         "$n$",
         "Memory Usage [MB]",
         "memory_type0_<>.pdf",
         ["Property_2", "Property_1", 'mdd_memory_consumption'],
         ["n-div-8", "n-div-4", "3n-div-8", "n-div-2", "5n-div-8", "3n-div-4", "7n-div-8"],
         "Property_2",
         "Property_1",
         div_fraction_format
         )

df1 = type1_df()
box_plot(df1,
         "Runtime Statistics for ",
         "$|\Sigma|$",
         "Runtime [s]",
         "runtime_type1_<>reps.pdf",
         ["Property_2", "Property_1", 'solution_runtime'],
         ["3", "4", "5", "6", "7", "8"],
         "Property_2",
         "Property_1",
         repetitions_format
         )

box_plot(df1,
         "Solution Length Statistics for ",
         "$|\Sigma|$",
         "RFLCS length",
         "solution_type1_<>reps.pdf",
         ["Property_2", "Property_1", 'solution_length'],
         ["3", "4", "5", "6", "7", "8"],
         "Property_2",
         "Property_1",
         repetitions_format
         )

box_plot(df1,
         "MDD Memory Usage Statistics for ",
         "$|\Sigma|$",
         "Memory Usage [MB]",
         "memory_type1_<>reps.pdf",
         ["Property_2", "Property_1", 'mdd_memory_consumption'],
         ["3", "4", "5", "6", "7", "8"],
         "Property_2",
         "Property_1",
         repetitions_format
         )