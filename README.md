# Single_Threaded_Image_Web_Crawler
## Objective
The findpng3 is a tiny, simplified, single-threaded concurrent web crawler that utilizes non-blocking I/O which enables simultaneous transfers. It searches the web starting from a seed URL. It then finds the number of valid PNG URLs that user specifies in the input or up to 50 PNG URLs and log them into a log.txt file. 

This time, the solution should not use pthreads. However, it should keep multiple concurrent connections to servers open.

## Sample Test Run
-t: Create t number of threads simultaneously crawling the web
<br> -m: Find up to m number of unique PNG URLs on the web
<br> -v: Name of log file, log all the visited URLs by the crawler, one URL per line in logfile

* **Input**: findpng2 -t 10 -m 20 -v log.txt http://ece252-1.uwaterloo.ca/lab4
* **Output**: findpng2 execution time: 10.123456 seconds

Log file may look like the following:
<br>http://ece252-1.uwaterloo.ca/lab4
<br>http://ece252-1.uwaterloo.ca/~yqhuang/lab4/#top
<br>http://ece252-1.uwaterloo.ca/~yqhuang/lab4/imgs/cpp.jpg
<br>http://ece252-1.uwaterloo.ca:2540/image?q=gAAAAABdHkoqPlVGPQPMpFG8Vjvrodu_pZqXMC4jGRiLcYwY6MkhhFG1m5a3x5ZYDOiLGFmz8FTbq3sAva7QKiXY5YNIxrBdxg==
<br>http://ece252-1.uwaterloo.ca/~yqhuang/lab4/imgs/para.jpg
<br>http://ece252-1.uwaterloo.ca/~yqhuang/lab4/imgs/crawler_arch.jpg
<br>http://ece252-1.uwaterloo.ca/~yqhuang/lab4/imgs/Disguise.png
<br>http://ece252-1.uwaterloo.ca:2531/image?img=1&part=0
<br>http://ece252-3.uwaterloo.ca:2540/image?q=gAAAAABdHkoqOKR-cFRCkiCUBEMAAAWfDvBFlRisL9ysLWHYHbcQbn1b28PV_uHBZ0gJf5bvzrnf1HNXxB6KRlAVETwTIqBH2Q==
<br>http://ece252-1.uwaterloo.ca/~yqhuang/lab4/sub1/index.html
<br>http://ece252-2.uwaterloo.ca:2540/image?q=gAAAAABdHkoqvYBSFk4VKGcRmB2TKYcl7hgNpTehsR0y7cd1ysTzvlgowzkgpWgwp42BumobA22jAEl0LJj_TcwR-X_F79vyuA==
<br>http://ece252-1.uwaterloo.ca:2540/image?q=gAAAAABdHkoq1GsX73XwnpU1qaO1laOtmzl_e3npHIX83pNGFgfOLJG2vwICDD5Gnb7iwdzV_ZKnckzZQfhsmEEeWbvO3-4Kug==
<br>http://ece252-1.uwaterloo.ca:2540/image?q=gAAAAABdHkoqXCN_I78zg0zDjOvOhA19H7tB5L2pSkuG0NmXg4E1LCe1Ew8utkFxulC2ssJUGOYgguCCXcDlOraPK-tYzsMl-w==
<br>http://ece252-3.uwaterloo.ca:2540/image?q=gAAAAABdHkoq5CAThsWSs_snhLDy2vDhV97ea1remux8ERu-H5FIIqoG5by17Zt7ba3hdadbOLXoHKJEYVHWdlvkpZg6VxeyaQ==
<br>http://ece252-1.uwaterloo.ca:2540/image?q=gAAAAABdHkoqKjtpjpiM4WKtVvo33HojmbxU3Q_VNRl2bS5uYIta1fUv23182WN7nHtTa-qg_M33g_5CvFk-5i95lVgkaDOCYA==
<br>http://ece252-2.uwaterloo.ca:2540/image?q=gAAAAABdHkoqymQU_88e8PRLFAHlee_iqRtsEkeVQsUV1YzN2c7mQXV1Sg7hOuNp212fkLFH71-EtoQMS_7bivH4V_Q5rx2Cfg==
<br>http://ece252-3.uwaterloo.ca:2540/image?q=gAAAAABdHkoqvsHZzgOGhVZBuBxA8IkeZS6GevG48xd-O9--4La4GPLs4qMW8QWW63iNSc4bypjbMJK1tZZGvHELGeVTmaoyZw==
<br>http://ece252-2.uwaterloo.ca:2540/image?q=gAAAAABdHlHZgxhjnyyylvmstitqthsafcipehodfzmjbugcviblrqxfzjgucrcllazmyhznzdkcmymdknquttkinbgbtshojf==
<br>http://ece252-3.uwaterloo.ca:2540/image?q=gAAAAABdHkoqdRxJhdCk49ofJ1TCxsbV6DF4DIHKtdWsF-5fJM0BHF6b6oM5Y99T7T78SQQcii0YvDMqzGN4L8a4eFsQotIHLA==
<br>http://ece252-3.uwaterloo.ca:2540/image?q=gAAAAABdHkoqu2HgO66gfwYQjCHJninMtqjg83iaDpT6U54p8F9XGbboHeGjhalG9fE7CMNx7vkgZZMzN-9c6EqVE5qXlhdqnw==
<br>http://ece252-2.uwaterloo.ca:2540/image?q=gAAAAABdHkoqMW24vZRpDznRyQZuRsR5vjPThROSj7k52y1weIHxIKOvt9xOxAbpZNudfTkojMyk2ii5Ap5o7e4cRdq_5C0aKw==
<br>http://ece252-2.uwaterloo.ca:2540/image?q=gAAAAABdHkoqLkzOa89N8w0I4kdmfUtjcN1D6AZYEOJCzr6qpnWae4LfzfIhCRH_MOeQj8x8_ugiRcF79z5D723Ssnm7U-84og==
<br>http://ece252-1.uwaterloo.ca:2540/image?q=gAAAAABdHkoqBsj81dnixZFLtjKlW6lHD5o0y0IWPRDcP0L_uEzcHVKXT6L-32Cl4u-th38LksMevU6T3NRufm6yVtFON0tXHg==
<br>http://ece252-1.uwaterloo.ca:2540/image?q=gAAAAABdHkoqYBTxXgfsZAuOV3Nkm0obQ58hGNAnrTrEDwPseWO4OKq2JXTnwAhg1lufmc4OhP7Vtf4hMEgzNdxNdeAgjsscVg==
<br>http://ece252-1.uwaterloo.ca:2540/image?q=gAAAAABdHkoq-JvNAjwE7fa5r26FVJgLMEqvkrj6AlO2BKI5zQtKRThKL79uHBoP-uRzt9WzYyPeec6zE6bmdkzy5UXt5Br2Iw==
<br>http://ece252-2.uwaterloo.ca:2540/image?q=gAAAAABdHkoq292-4eAqc4xZ-nwNibr1F5ZZCJhojZGV7hQMtNzryLwR8grsztNSurbqnucszcyMbtSE8BaFg049sI6WZ4aDfg==
<br>http://ece252-1.uwaterloo.ca:2540/image?q=gAAAAABdHkoqtuljtcVzSazkwT4SVEw_rlEYwknlCq6mKLqndncMei2Om8ccO2ieak5QQCVNJKuwG5FYmX8sgbhKht2xt4ZNlw==

