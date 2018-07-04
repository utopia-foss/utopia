"""Plotting tools that can be used from all model-specific plot functions"""

import matplotlib.pyplot as plt

# -----------------------------------------------------------------------------

def save_and_close(out_path: str, *, save_kwargs: dict=None) -> None:
    """Save and close the figure, passing the kwargs
    
    Args:
        out_path (str): The output path to save the figure to
        save_kwargs (dict, optional): The additional save_kwargs
    """
    plt.savefig(out_path, **(save_kwargs if save_kwargs else {}))
    plt.close()
