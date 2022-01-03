import socket
import d3dshot
import numpy as np
import time
import matplotlib.pyplot as plt


def rgb_running_mean(x, k):
    """
    Calculates the running mean of a (n*3) rgb array.
    :param x: The array to calculate the running mean of.
    :param k: The kernel to use for the running mean.
    """
    x_new = np.zeros(x.shape)
    x_new[:, 0] = np.convolve(x[:, 0], k, mode="same")
    x_new[:, 1] = np.convolve(x[:, 1], k, mode="same")
    x_new[:, 2] = np.convolve(x[:, 2], k, mode="same")
    return x_new

def cut_img_divisible(img, div):
    """
    Cuts an image to make it divisible by a number along axis 0.
    :param img: The image to cut.
    :param div: The number to cut the image by.
    :return: The cut image. 
    """
    s_div = (img.shape[0] % div)//2
    img = img[s_div:-s_div, :]
    return img

#def calc_rgb_vals(screencap, plot_test_img=False):
def calc_rgb_vals(screencap, plot_test_img=False):
    """
    Calculates the RGB values of the screencap.
    LEDs go from bottom right to top right, top right to top left, top left to bottom left.
    Return format is a numpy array of shape (3, num_leds)
    :param screencap: The screencap as a numpy array.
    :param plot_test_img: If true, plots the screencap.
    :return: The RGB values of the screencap.
    """

    border_width = 300
    n_leds_short = 35
    n_leds_long = 72

    kernel_long = np.ones(20) / 20
    kernel_short = np.ones(10) / 10
    # avg and smooth the screen borders
    avg_top = np.mean(screencap[:border_width, :], axis=0)
    avg_top = rgb_running_mean(avg_top, kernel_long)
    avg_left = np.mean(screencap[:, :border_width], axis=1)
    avg_left = rgb_running_mean(avg_left, kernel_short)
    avg_right = np.mean(screencap[:, -border_width:], axis=1)
    avg_right = rgb_running_mean(avg_right, kernel_short)
    
    # make screen size divisible by number of leds
    avg_top = cut_img_divisible(avg_top, n_leds_long)
    avg_left = cut_img_divisible(avg_left, n_leds_short)
    avg_right = cut_img_divisible(avg_right, n_leds_short)

    # avg the pixels for each LED
    vals_top = (avg_top.reshape((n_leds_long, -1, 3)).mean(axis=1)).astype(int)
    vals_left = (avg_left.reshape((n_leds_short, -1, 3)).mean(axis=1)).astype(int)
    vals_right = (avg_right.reshape((n_leds_short, -1, 3)).mean(axis=1)).astype(int)

    # test image to plot zones
    if plot_test_img:
        img_test = np.zeros((n_leds_short, n_leds_long, 3), dtype=int)
        img_test[0, :,:] = vals_top
        img_test[:, 0, :] = vals_left
        img_test[:, -1, :] = vals_right
        plt.matshow(img_test)
        plt.show()
    # format the return so it matches the led setup. mine is bot right -> top right -> top left -> bot left
    return np.concatenate([vals_right[::-1], vals_top[::-1], vals_left])


def main():
    UDP_IP = "192.168.178.32"
    UDP_PORT = 4210
    print("UDP target IP: %s" % UDP_IP)
    print("UDP target port: %s" % UDP_PORT)
    with socket.socket(socket.AF_INET, socket.SOCK_DGRAM) as sock:
        d = d3dshot.create(capture_output="numpy")
        out = d.screenshot()
        while 1:
            t0 = time.time()
            out = d.screenshot()
            rgb_vals = calc_rgb_vals(out, plot_test_img=0)
            MESSAGE = bytes(list(rgb_vals.flatten()))
            sock.sendto(MESSAGE, (UDP_IP, UDP_PORT))
            print(f"Processing took {(time.time()-t0)}")
            time.sleep(0.0166) # ~60 FPS, need to limit this when using 144 LEDs or ESP cant handle

if __name__ == "__main__":
    main()