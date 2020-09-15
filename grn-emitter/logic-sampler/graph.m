size = [1,1,2,2,3,3,3,4,4,4,5,5,6,6,7,7,8,8,9,9,10,10];
FUs = [1,2,4,5,7,7,8,10,7,13,16,14,21,18,24,22,20,27,31,25,32,33];
RPs = [0,1,2,4,5,5,6,9,4,11,15,12,20,16,23,21,17,25,30,22,28,33];

figure('PaperPosition',[1 1 4 3]);
plot(size,FUs,'b*'); hold on;
plot(size,RPs,'r^');
xlabel('# sensor inputs');
ylabel('# computational elements');
legend('Promoters','Transcription Factors','Location','NorthWest');
title('Optimized Circuit Complexity');
print('-depsc','circuit-complexity.eps');
print('-dpdf','circuit-complexity.pdf');
