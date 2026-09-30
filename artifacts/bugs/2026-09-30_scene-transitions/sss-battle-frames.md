# sss-battle: every consecutive captured scanout

The tic column is the source tic currently in progress at the IRQ stop, not a claim that its OAM has already committed. PNG pixels identify the last complete scanout. `mixed` includes previous-scene SUB telemetry when MAIN has changed; descriptions make that case explicit.

| Index / both-screen PNG | VBlank | Present counter | Results tic in progress | Old scene during load | Mixed | Stale 3D | Incomplete | Main pixels | Sub pixels |
|---|---:|---:|---:|---|---|---|---|---|---|
| [0000](r58-sequence01/sss-battle-0000.png) | 668 | 429 | 0 | no | no | no | no | SSS retained | SSS telemetry |
| [0001](r58-sequence01/sss-battle-0001.png) | 669 | 430 | 0 | no | no | no | no | SSS retained | SSS telemetry |
| [0002](r58-sequence01/sss-battle-0002.png) | 670 | 430 | 0 | no | no | no | no | SSS retained | SSS telemetry |
| [0003](r58-sequence01/sss-battle-0003.png) | 671 | 432 | 0 | no | no | no | no | SSS retained | SSS telemetry |
| [0004](r58-sequence01/sss-battle-0004.png) | 672 | 432 | 0 | no | no | no | no | SSS retained | SSS telemetry |
| [0005](r58-sequence01/sss-battle-0005.png) | 673 | 434 | 0 | no | no | no | no | SSS retained | SSS telemetry |
| [0006](r58-sequence01/sss-battle-0006.png) | 674 | 435 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0007](r58-sequence01/sss-battle-0007.png) | 675 | 436 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0008](r58-sequence01/sss-battle-0008.png) | 676 | 437 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0009](r58-sequence01/sss-battle-0009.png) | 677 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0010](r58-sequence01/sss-battle-0010.png) | 678 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0011](r58-sequence01/sss-battle-0011.png) | 679 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0012](r58-sequence01/sss-battle-0012.png) | 680 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0013](r58-sequence01/sss-battle-0013.png) | 681 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0014](r58-sequence01/sss-battle-0014.png) | 682 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0015](r58-sequence01/sss-battle-0015.png) | 683 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0016](r58-sequence01/sss-battle-0016.png) | 684 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0017](r58-sequence01/sss-battle-0017.png) | 685 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0018](r58-sequence01/sss-battle-0018.png) | 686 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0019](r58-sequence01/sss-battle-0019.png) | 687 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0020](r58-sequence01/sss-battle-0020.png) | 688 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0021](r58-sequence01/sss-battle-0021.png) | 689 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0022](r58-sequence01/sss-battle-0022.png) | 690 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0023](r58-sequence01/sss-battle-0023.png) | 691 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0024](r58-sequence01/sss-battle-0024.png) | 692 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0025](r58-sequence01/sss-battle-0025.png) | 693 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0026](r58-sequence01/sss-battle-0026.png) | 694 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0027](r58-sequence01/sss-battle-0027.png) | 695 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0028](r58-sequence01/sss-battle-0028.png) | 696 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0029](r58-sequence01/sss-battle-0029.png) | 697 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0030](r58-sequence01/sss-battle-0030.png) | 698 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0031](r58-sequence01/sss-battle-0031.png) | 699 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0032](r58-sequence01/sss-battle-0032.png) | 700 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0033](r58-sequence01/sss-battle-0033.png) | 701 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0034](r58-sequence01/sss-battle-0034.png) | 702 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0035](r58-sequence01/sss-battle-0035.png) | 703 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0036](r58-sequence01/sss-battle-0036.png) | 704 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0037](r58-sequence01/sss-battle-0037.png) | 705 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0038](r58-sequence01/sss-battle-0038.png) | 706 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0039](r58-sequence01/sss-battle-0039.png) | 707 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0040](r58-sequence01/sss-battle-0040.png) | 708 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0041](r58-sequence01/sss-battle-0041.png) | 709 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0042](r58-sequence01/sss-battle-0042.png) | 710 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0043](r58-sequence01/sss-battle-0043.png) | 711 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0044](r58-sequence01/sss-battle-0044.png) | 712 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0045](r58-sequence01/sss-battle-0045.png) | 713 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0046](r58-sequence01/sss-battle-0046.png) | 714 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0047](r58-sequence01/sss-battle-0047.png) | 715 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0048](r58-sequence01/sss-battle-0048.png) | 716 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0049](r58-sequence01/sss-battle-0049.png) | 717 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0050](r58-sequence01/sss-battle-0050.png) | 718 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0051](r58-sequence01/sss-battle-0051.png) | 719 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0052](r58-sequence01/sss-battle-0052.png) | 720 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0053](r58-sequence01/sss-battle-0053.png) | 721 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0054](r58-sequence01/sss-battle-0054.png) | 722 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0055](r58-sequence01/sss-battle-0055.png) | 723 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0056](r58-sequence01/sss-battle-0056.png) | 724 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0057](r58-sequence01/sss-battle-0057.png) | 725 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0058](r58-sequence01/sss-battle-0058.png) | 726 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0059](r58-sequence01/sss-battle-0059.png) | 727 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0060](r58-sequence01/sss-battle-0060.png) | 728 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0061](r58-sequence01/sss-battle-0061.png) | 729 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0062](r58-sequence01/sss-battle-0062.png) | 730 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0063](r58-sequence01/sss-battle-0063.png) | 731 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0064](r58-sequence01/sss-battle-0064.png) | 732 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0065](r58-sequence01/sss-battle-0065.png) | 733 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0066](r58-sequence01/sss-battle-0066.png) | 734 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0067](r58-sequence01/sss-battle-0067.png) | 735 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0068](r58-sequence01/sss-battle-0068.png) | 736 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0069](r58-sequence01/sss-battle-0069.png) | 737 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0070](r58-sequence01/sss-battle-0070.png) | 738 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0071](r58-sequence01/sss-battle-0071.png) | 739 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0072](r58-sequence01/sss-battle-0072.png) | 740 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0073](r58-sequence01/sss-battle-0073.png) | 741 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0074](r58-sequence01/sss-battle-0074.png) | 742 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0075](r58-sequence01/sss-battle-0075.png) | 743 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0076](r58-sequence01/sss-battle-0076.png) | 744 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0077](r58-sequence01/sss-battle-0077.png) | 745 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0078](r58-sequence01/sss-battle-0078.png) | 746 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0079](r58-sequence01/sss-battle-0079.png) | 747 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0080](r58-sequence01/sss-battle-0080.png) | 748 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0081](r58-sequence01/sss-battle-0081.png) | 749 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0082](r58-sequence01/sss-battle-0082.png) | 750 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0083](r58-sequence01/sss-battle-0083.png) | 751 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0084](r58-sequence01/sss-battle-0084.png) | 752 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0085](r58-sequence01/sss-battle-0085.png) | 753 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0086](r58-sequence01/sss-battle-0086.png) | 754 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0087](r58-sequence01/sss-battle-0087.png) | 755 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0088](r58-sequence01/sss-battle-0088.png) | 756 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0089](r58-sequence01/sss-battle-0089.png) | 757 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0090](r58-sequence01/sss-battle-0090.png) | 758 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0091](r58-sequence01/sss-battle-0091.png) | 759 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0092](r58-sequence01/sss-battle-0092.png) | 760 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0093](r58-sequence01/sss-battle-0093.png) | 761 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0094](r58-sequence01/sss-battle-0094.png) | 762 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0095](r58-sequence01/sss-battle-0095.png) | 763 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0096](r58-sequence01/sss-battle-0096.png) | 764 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0097](r58-sequence01/sss-battle-0097.png) | 765 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0098](r58-sequence01/sss-battle-0098.png) | 766 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0099](r58-sequence01/sss-battle-0099.png) | 767 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0100](r58-sequence01/sss-battle-0100.png) | 768 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0101](r58-sequence01/sss-battle-0101.png) | 769 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0102](r58-sequence01/sss-battle-0102.png) | 770 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0103](r58-sequence01/sss-battle-0103.png) | 771 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0104](r58-sequence01/sss-battle-0104.png) | 772 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0105](r58-sequence01/sss-battle-0105.png) | 773 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0106](r58-sequence01/sss-battle-0106.png) | 774 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0107](r58-sequence01/sss-battle-0107.png) | 775 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0108](r58-sequence01/sss-battle-0108.png) | 776 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0109](r58-sequence01/sss-battle-0109.png) | 777 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0110](r58-sequence01/sss-battle-0110.png) | 778 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0111](r58-sequence01/sss-battle-0111.png) | 779 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0112](r58-sequence01/sss-battle-0112.png) | 780 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0113](r58-sequence01/sss-battle-0113.png) | 781 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0114](r58-sequence01/sss-battle-0114.png) | 782 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0115](r58-sequence01/sss-battle-0115.png) | 783 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0116](r58-sequence01/sss-battle-0116.png) | 784 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0117](r58-sequence01/sss-battle-0117.png) | 785 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0118](r58-sequence01/sss-battle-0118.png) | 786 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0119](r58-sequence01/sss-battle-0119.png) | 787 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0120](r58-sequence01/sss-battle-0120.png) | 788 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0121](r58-sequence01/sss-battle-0121.png) | 789 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0122](r58-sequence01/sss-battle-0122.png) | 790 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0123](r58-sequence01/sss-battle-0123.png) | 791 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0124](r58-sequence01/sss-battle-0124.png) | 792 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0125](r58-sequence01/sss-battle-0125.png) | 793 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0126](r58-sequence01/sss-battle-0126.png) | 794 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0127](r58-sequence01/sss-battle-0127.png) | 795 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0128](r58-sequence01/sss-battle-0128.png) | 796 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0129](r58-sequence01/sss-battle-0129.png) | 797 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0130](r58-sequence01/sss-battle-0130.png) | 798 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0131](r58-sequence01/sss-battle-0131.png) | 799 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0132](r58-sequence01/sss-battle-0132.png) | 800 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0133](r58-sequence01/sss-battle-0133.png) | 801 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0134](r58-sequence01/sss-battle-0134.png) | 802 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0135](r58-sequence01/sss-battle-0135.png) | 803 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0136](r58-sequence01/sss-battle-0136.png) | 804 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0137](r58-sequence01/sss-battle-0137.png) | 805 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0138](r58-sequence01/sss-battle-0138.png) | 806 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0139](r58-sequence01/sss-battle-0139.png) | 807 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0140](r58-sequence01/sss-battle-0140.png) | 808 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0141](r58-sequence01/sss-battle-0141.png) | 809 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0142](r58-sequence01/sss-battle-0142.png) | 810 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0143](r58-sequence01/sss-battle-0143.png) | 811 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0144](r58-sequence01/sss-battle-0144.png) | 812 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0145](r58-sequence01/sss-battle-0145.png) | 813 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0146](r58-sequence01/sss-battle-0146.png) | 814 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0147](r58-sequence01/sss-battle-0147.png) | 815 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0148](r58-sequence01/sss-battle-0148.png) | 816 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0149](r58-sequence01/sss-battle-0149.png) | 817 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0150](r58-sequence01/sss-battle-0150.png) | 818 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0151](r58-sequence01/sss-battle-0151.png) | 819 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0152](r58-sequence01/sss-battle-0152.png) | 820 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0153](r58-sequence01/sss-battle-0153.png) | 821 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0154](r58-sequence01/sss-battle-0154.png) | 822 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0155](r58-sequence01/sss-battle-0155.png) | 823 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0156](r58-sequence01/sss-battle-0156.png) | 824 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0157](r58-sequence01/sss-battle-0157.png) | 825 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0158](r58-sequence01/sss-battle-0158.png) | 826 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0159](r58-sequence01/sss-battle-0159.png) | 827 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0160](r58-sequence01/sss-battle-0160.png) | 828 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0161](r58-sequence01/sss-battle-0161.png) | 829 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0162](r58-sequence01/sss-battle-0162.png) | 830 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0163](r58-sequence01/sss-battle-0163.png) | 831 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0164](r58-sequence01/sss-battle-0164.png) | 832 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0165](r58-sequence01/sss-battle-0165.png) | 833 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0166](r58-sequence01/sss-battle-0166.png) | 834 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0167](r58-sequence01/sss-battle-0167.png) | 835 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0168](r58-sequence01/sss-battle-0168.png) | 836 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0169](r58-sequence01/sss-battle-0169.png) | 837 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0170](r58-sequence01/sss-battle-0170.png) | 838 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0171](r58-sequence01/sss-battle-0171.png) | 839 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0172](r58-sequence01/sss-battle-0172.png) | 840 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0173](r58-sequence01/sss-battle-0173.png) | 841 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0174](r58-sequence01/sss-battle-0174.png) | 842 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0175](r58-sequence01/sss-battle-0175.png) | 843 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0176](r58-sequence01/sss-battle-0176.png) | 844 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0177](r58-sequence01/sss-battle-0177.png) | 845 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0178](r58-sequence01/sss-battle-0178.png) | 846 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0179](r58-sequence01/sss-battle-0179.png) | 847 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0180](r58-sequence01/sss-battle-0180.png) | 848 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0181](r58-sequence01/sss-battle-0181.png) | 849 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0182](r58-sequence01/sss-battle-0182.png) | 850 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0183](r58-sequence01/sss-battle-0183.png) | 851 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0184](r58-sequence01/sss-battle-0184.png) | 852 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0185](r58-sequence01/sss-battle-0185.png) | 853 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0186](r58-sequence01/sss-battle-0186.png) | 854 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0187](r58-sequence01/sss-battle-0187.png) | 855 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0188](r58-sequence01/sss-battle-0188.png) | 856 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0189](r58-sequence01/sss-battle-0189.png) | 857 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0190](r58-sequence01/sss-battle-0190.png) | 858 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0191](r58-sequence01/sss-battle-0191.png) | 859 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0192](r58-sequence01/sss-battle-0192.png) | 860 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0193](r58-sequence01/sss-battle-0193.png) | 861 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0194](r58-sequence01/sss-battle-0194.png) | 862 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0195](r58-sequence01/sss-battle-0195.png) | 863 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0196](r58-sequence01/sss-battle-0196.png) | 864 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0197](r58-sequence01/sss-battle-0197.png) | 865 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0198](r58-sequence01/sss-battle-0198.png) | 866 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0199](r58-sequence01/sss-battle-0199.png) | 867 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0200](r58-sequence01/sss-battle-0200.png) | 868 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0201](r58-sequence01/sss-battle-0201.png) | 869 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0202](r58-sequence01/sss-battle-0202.png) | 870 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0203](r58-sequence01/sss-battle-0203.png) | 871 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0204](r58-sequence01/sss-battle-0204.png) | 872 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0205](r58-sequence01/sss-battle-0205.png) | 873 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0206](r58-sequence01/sss-battle-0206.png) | 874 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0207](r58-sequence01/sss-battle-0207.png) | 875 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0208](r58-sequence01/sss-battle-0208.png) | 876 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0209](r58-sequence01/sss-battle-0209.png) | 877 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0210](r58-sequence01/sss-battle-0210.png) | 878 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0211](r58-sequence01/sss-battle-0211.png) | 879 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0212](r58-sequence01/sss-battle-0212.png) | 880 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0213](r58-sequence01/sss-battle-0213.png) | 881 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0214](r58-sequence01/sss-battle-0214.png) | 882 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0215](r58-sequence01/sss-battle-0215.png) | 883 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0216](r58-sequence01/sss-battle-0216.png) | 884 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0217](r58-sequence01/sss-battle-0217.png) | 885 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0218](r58-sequence01/sss-battle-0218.png) | 886 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0219](r58-sequence01/sss-battle-0219.png) | 887 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0220](r58-sequence01/sss-battle-0220.png) | 888 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0221](r58-sequence01/sss-battle-0221.png) | 889 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0222](r58-sequence01/sss-battle-0222.png) | 890 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0223](r58-sequence01/sss-battle-0223.png) | 891 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0224](r58-sequence01/sss-battle-0224.png) | 892 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0225](r58-sequence01/sss-battle-0225.png) | 893 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0226](r58-sequence01/sss-battle-0226.png) | 894 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0227](r58-sequence01/sss-battle-0227.png) | 895 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0228](r58-sequence01/sss-battle-0228.png) | 896 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0229](r58-sequence01/sss-battle-0229.png) | 897 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0230](r58-sequence01/sss-battle-0230.png) | 898 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0231](r58-sequence01/sss-battle-0231.png) | 899 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0232](r58-sequence01/sss-battle-0232.png) | 900 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0233](r58-sequence01/sss-battle-0233.png) | 901 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0234](r58-sequence01/sss-battle-0234.png) | 902 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0235](r58-sequence01/sss-battle-0235.png) | 903 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0236](r58-sequence01/sss-battle-0236.png) | 904 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0237](r58-sequence01/sss-battle-0237.png) | 905 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0238](r58-sequence01/sss-battle-0238.png) | 906 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0239](r58-sequence01/sss-battle-0239.png) | 907 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0240](r58-sequence01/sss-battle-0240.png) | 908 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0241](r58-sequence01/sss-battle-0241.png) | 909 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0242](r58-sequence01/sss-battle-0242.png) | 910 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0243](r58-sequence01/sss-battle-0243.png) | 911 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0244](r58-sequence01/sss-battle-0244.png) | 912 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0245](r58-sequence01/sss-battle-0245.png) | 913 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0246](r58-sequence01/sss-battle-0246.png) | 914 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0247](r58-sequence01/sss-battle-0247.png) | 915 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0248](r58-sequence01/sss-battle-0248.png) | 916 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0249](r58-sequence01/sss-battle-0249.png) | 917 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0250](r58-sequence01/sss-battle-0250.png) | 918 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0251](r58-sequence01/sss-battle-0251.png) | 919 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0252](r58-sequence01/sss-battle-0252.png) | 920 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0253](r58-sequence01/sss-battle-0253.png) | 921 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0254](r58-sequence01/sss-battle-0254.png) | 922 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0255](r58-sequence01/sss-battle-0255.png) | 923 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0256](r58-sequence01/sss-battle-0256.png) | 924 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0257](r58-sequence01/sss-battle-0257.png) | 925 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0258](r58-sequence01/sss-battle-0258.png) | 926 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0259](r58-sequence01/sss-battle-0259.png) | 927 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0260](r58-sequence01/sss-battle-0260.png) | 928 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0261](r58-sequence01/sss-battle-0261.png) | 929 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0262](r58-sequence01/sss-battle-0262.png) | 930 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0263](r58-sequence01/sss-battle-0263.png) | 931 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0264](r58-sequence01/sss-battle-0264.png) | 932 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0265](r58-sequence01/sss-battle-0265.png) | 933 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0266](r58-sequence01/sss-battle-0266.png) | 934 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0267](r58-sequence01/sss-battle-0267.png) | 935 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0268](r58-sequence01/sss-battle-0268.png) | 936 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0269](r58-sequence01/sss-battle-0269.png) | 937 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0270](r58-sequence01/sss-battle-0270.png) | 938 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0271](r58-sequence01/sss-battle-0271.png) | 939 | 438 | 0 | **YES** | no | no | no | SSS retained | SSS telemetry |
| [0272](r58-sequence01/sss-battle-0272.png) | 940 | 438 | 0 | **YES** | **YES** | no | **YES** | partial Castle wallpaper over cleared field | SSS telemetry |
| [0273](r58-sequence01/sss-battle-0273.png) | 941 | 438 | 0 | **YES** | **YES** | no | **YES** | partial Castle wallpaper over cleared field | SSS telemetry |
| [0274](r58-sequence01/sss-battle-0274.png) | 942 | 438 | 0 | **YES** | **YES** | no | **YES** | partial Castle wallpaper over cleared field | SSS telemetry |
| [0275](r58-sequence01/sss-battle-0275.png) | 943 | 438 | 0 | **YES** | **YES** | no | **YES** | partial Castle wallpaper over cleared field | SSS telemetry |
| [0276](r58-sequence01/sss-battle-0276.png) | 944 | 438 | 0 | **YES** | **YES** | no | **YES** | partial Castle wallpaper over cleared field | SSS telemetry |
| [0277](r58-sequence01/sss-battle-0277.png) | 945 | 438 | 0 | **YES** | **YES** | no | **YES** | partial Castle wallpaper over cleared field | SSS telemetry |
| [0278](r58-sequence01/sss-battle-0278.png) | 946 | 438 | 0 | **YES** | **YES** | no | **YES** | Castle wallpaper only | SSS telemetry |
| [0279](r58-sequence01/sss-battle-0279.png) | 947 | 438 | 0 | **YES** | **YES** | no | **YES** | Castle wallpaper only | SSS telemetry |
| [0280](r58-sequence01/sss-battle-0280.png) | 948 | 438 | 0 | **YES** | **YES** | no | **YES** | Castle wallpaper only | SSS telemetry |
| [0281](r58-sequence01/sss-battle-0281.png) | 949 | 438 | 0 | **YES** | **YES** | no | **YES** | Castle wallpaper only | SSS telemetry |
| [0282](r58-sequence01/sss-battle-0282.png) | 950 | 438 | 0 | **YES** | **YES** | no | **YES** | Castle wallpaper only | SSS telemetry |
| [0283](r58-sequence01/sss-battle-0283.png) | 951 | 438 | 0 | **YES** | **YES** | no | **YES** | Castle wallpaper only | SSS telemetry |
| [0284](r58-sequence01/sss-battle-0284.png) | 952 | 438 | 0 | **YES** | **YES** | no | **YES** | Castle wallpaper only | SSS telemetry |
| [0285](r58-sequence01/sss-battle-0285.png) | 953 | 438 | 0 | **YES** | **YES** | no | **YES** | Castle wallpaper only | SSS telemetry |
| [0286](r58-sequence01/sss-battle-0286.png) | 954 | 438 | 0 | **YES** | **YES** | no | **YES** | Castle wallpaper only | SSS telemetry |
| [0287](r58-sequence01/sss-battle-0287.png) | 955 | 438 | 0 | **YES** | **YES** | no | **YES** | Castle wallpaper only | SSS telemetry |
| [0288](r58-sequence01/sss-battle-0288.png) | 956 | 438 | 0 | **YES** | **YES** | no | **YES** | Castle wallpaper only | SSS telemetry |
| [0289](r58-sequence01/sss-battle-0289.png) | 957 | 438 | 0 | **YES** | **YES** | no | **YES** | Castle wallpaper only | SSS telemetry |
| [0290](r58-sequence01/sss-battle-0290.png) | 958 | 438 | 0 | **YES** | **YES** | no | **YES** | Castle wallpaper only | SSS telemetry |
| [0291](r58-sequence01/sss-battle-0291.png) | 959 | 438 | 0 | **YES** | **YES** | no | **YES** | Castle wallpaper only | SSS telemetry |
| [0292](r58-sequence01/sss-battle-0292.png) | 960 | 438 | 0 | **YES** | **YES** | no | **YES** | Castle wallpaper only | SSS telemetry |
| [0293](r58-sequence01/sss-battle-0293.png) | 961 | 438 | 0 | no | no | no | **YES** | Castle wallpaper, no stage geometry | battle telemetry/HUD |
| [0294](r58-sequence01/sss-battle-0294.png) | 962 | 439 | 0 | no | **YES** | **YES** | **YES** | retained CSS Mario/Fox over Castle wallpaper | battle telemetry, HUD starting |
| [0295](r58-sequence01/sss-battle-0295.png) | 963 | 439 | 0 | no | no | no | no | Castle battle stage under source entry fade | battle telemetry/HUD |
| [0296](r58-sequence01/sss-battle-0296.png) | 964 | 440 | 0 | no | no | no | no | Castle battle stage under source entry fade | battle telemetry/HUD |
| [0297](r58-sequence01/sss-battle-0297.png) | 965 | 440 | 0 | no | no | no | no | Castle battle stage under source entry fade | battle telemetry/HUD |
| [0298](r58-sequence01/sss-battle-0298.png) | 966 | 441 | 0 | no | no | no | no | Castle battle stage under source entry fade | battle telemetry/HUD |
| [0299](r58-sequence01/sss-battle-0299.png) | 967 | 441 | 0 | no | no | no | no | Castle battle stage under source entry fade | battle telemetry/HUD |
| [0300](r58-sequence01/sss-battle-0300.png) | 968 | 442 | 0 | no | no | no | no | Castle battle stage under source entry fade | battle telemetry/HUD |
| [0301](r58-sequence01/sss-battle-0301.png) | 969 | 442 | 0 | no | no | no | no | Castle battle stage under source entry fade | battle telemetry/HUD |
| [0302](r58-sequence01/sss-battle-0302.png) | 970 | 443 | 0 | no | no | no | no | Castle battle stage under source entry fade | battle telemetry/HUD |
