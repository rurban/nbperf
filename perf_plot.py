# Taken from github.com/rurban/ctl
# MIT Licensed
# pip install plotly psutil kaleido

import plotly.graph_objs as go
#import plotly.express as px
import sys
import string

def lines_from_file(log_fname):
    with open(log_fname, "r") as f:
        file_raw = f.read()
    file_lines = file_raw.split("\n")
    return file_lines

def plot_data_from_file(file_lines):
    name_plot_hash = {}
    fname_base = None
    fname_ext = None
    for line in file_lines:
        if ":" in line:
            plot_name = line
            name_plot_hash[plot_name] = {
                "num_of_ints": [],
                "time": []
            }
        elif line != "":
            half_line_len = int(len(line)/2)
            x = int(line[:half_line_len])
            y = int(line[half_line_len:])
            name_plot_hash[plot_name]["num_of_ints"].append(x)
            name_plot_hash[plot_name]["time"].append(y / 1e6)
    return name_plot_hash

def plot_from_data(name_plot_hash, title):
   color_for_c = "#333"
   color_for_cpp = "#888"
   grid_color = "#C9C9C9"
   plot_bgcolor = "#F9F9F9"
   paper_bgcolor = "#F6F8FA"
   # lst has 6 items
   color_list = [
       "#003f5c", # dark blue
       "#006400", # green
       "#955196", # violet
       "#dd5182", # reddish
       "#ff6e54", # orange
       "#ffa600", # yellow-orange
       "#ffdf10",
       "#ffdff0",
   ]
   used_basenames = []
   trace_list = []
   for idx, name in enumerate(name_plot_hash.keys()):
       trace_color = color_list[int(idx / 2)]
       if ("--chm" in name) or ("--bpz" in name):
           is_mph = True
       else:
           is_mph = False
       trace = go.Scatter(
           x=name_plot_hash[name]["num_of_ints"],
           y=name_plot_hash[name]["time"],
           name=name[7:], # skip option:
           mode="lines",
           marker=dict(color=trace_color),
           line=dict(width=2, dash=("dot" if is_mph else "solid"))
       )
       trace_list.append(trace)
   fig = go.Figure(data=trace_list)
   fig.update_xaxes(type="log")
   if "sizes" in title:
       ylegend = "kB"
   else:
       ylegend = "Seconds"
   fig.update_layout(
       plot_bgcolor=plot_bgcolor,
       paper_bgcolor=paper_bgcolor,
       xaxis=dict(title="Size", nticks=40, showgrid=True, gridcolor=grid_color),
       yaxis=dict(title=ylegend, showgrid=True, gridcolor=grid_color, type="log"),
       title=title,
       legend=dict(yanchor="top", xanchor="left", x=0.01, y=-0.1)
   )
   return fig

if __name__ == "__main__":
    filename = sys.argv[1]
    title = sys.argv[2]
    file_lines = lines_from_file(filename)
    name_plot_hash = plot_data_from_file(file_lines)
    fig = plot_from_data(name_plot_hash, title)
    len = len(filename)
    basename = filename[:len - 4]
    fig.write_image("%s.png" % basename, width=740, height=650)
    fig.write_image("%s.svg" % basename, width=740, height=650)
