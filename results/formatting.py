EPSILON = 0.0099
INFINITY = 99999999
MINUS_INFINITY = -INFINITY


def clamp_min(value):
    if not isinstance(value, float):
        return INFINITY
    if value < 0.01:
        return EPSILON
    return value


def clamp_max(value):
    if not isinstance(value, float):
        return MINUS_INFINITY
    return value


def string_replace_min(value):
    if not isinstance(value, float):
        return INFINITY
    return value


def clamp_format_min(value, alt_values):
    clamped_value = clamp_min(value)
    if clamped_value == INFINITY:
        return value
    if clamped_value == EPSILON:
        return "$\\mathbf{<0.01}$"
    clamped_min = min([clamp_min(alt_value) for alt_value in alt_values])
    formatted = f"{value:.2f}"
    if round(clamped_value, 2) <= round(clamped_min, 2):
        return '$\\mathbf{' + formatted + '}$'
    else:
        return formatted


def format_max(value, alt_values):
    clamped_value = clamp_max(value)
    if clamped_value == MINUS_INFINITY:
        return value
    formatted = f"{value:.2f}"
    alt_max = max([clamp_max(alt_value) for alt_value in alt_values])
    if round(clamped_value, 2) >= round(alt_max, 2):
        return '$\\mathbf{' + formatted + '}$'
    else:
        return formatted


def format_min(value, alt_values):
    if not isinstance(value, float):
        return value
    clamped_value = string_replace_min(value)
    if clamped_value == EPSILON:
        return '$\\mathbf{<0.01}$'
    alt_min = min([string_replace_min(alt_value) for alt_value in alt_values])
    formatted = f"{value:.2f}"
    if round(clamped_value, 2) <= round(alt_min, 2):
        return '$\\mathbf{' + formatted + '}$'
    else:
        return formatted


def format_opt(solved_format, reduction_format, solved_alt, reduction_alt):
    if solved_format == "-":
        return "-"
    if solved_alt == "-":
        solved_alt = -1
        reduction_alt = -1
    if round(solved_format, 2) >= round(solved_alt, 2) and round(reduction_format, 2) >= round(reduction_alt, 2):
        return "\\textbf{" + str(int(solved_format)) + "/" + str(int(reduction_format)) + "}"
    solved = str(int(solved_format))
    reduction = str(int(reduction_format))
    if round(solved_format, 2) >= round(solved_alt, 2):
        solved = "\\textbf{" + solved + "}"
    if round(reduction_format, 2) >= round(reduction_alt, 2):
        reduction = "\\textbf{" + reduction + "}"
    return solved + "/" + reduction


def float_format(value):
    if value < 0.01:
        return "$<0.01$"
    else:
        return f"{value:.2f}"


def bold_30(value):
    if value == 30:
        return "\\textbf{30}"
    else:
        return str(value)
