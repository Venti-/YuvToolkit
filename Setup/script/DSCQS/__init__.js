__setupPackage__("DSCQS");

playback_control = function () {
	this.counter = 0;
	this.lastFrameDisplayed = function() {
		this.counter ++;
		yt.closeAll()
		}
	this.reset_counter = function() {
		this.counter = 0
		}
};

sleep = function(t,f) {
  if (f === undefined) return;
  setTimeout(f,t);
}

DSCQS.test = function () {
	  this.randomShuffle = function(orig, chang, List){
        var temp = List[orig];
        List[orig] = List[chang];
        List[chang] = temp;
        return List;
    };
    this.randomizeList = function(ListScenes, lastFixedVideo){
				var last = lastFixedVideo || 0;
        len_test = ListScenes.length;
        len_scene = ListScenes[0].length;
        for (var i = len_test - 1; i > 0; i--) {
            //Randomize scene order
            var j = Math.floor(Math.random() * (i + 1));            
            ListScenes = this.randomShuffle(i, j, ListScenes);
            for (var k = len_scene - 1; k > last; k--){
                //Randomize video order. If lastFixedVideo != 0, only randomize videos between lastFixedVideo and end.
                var p = Math.max(Math.floor(Math.random() * (k + 1)), last+1);
                ListScenes[i] = this.randomShuffle(k, p, ListScenes[i]);
            }
        }
        return ListScenes;
    };
	this.start = function(InputList,black_3s) {
		this.videoList = this.randomizeList(InputList);
		this.current = -1;
		
		this.black_3s = black_3s;
		
		yt.PopNameInputWindow();
		this.username = yt.getNameInputUserName();
		this.resultfilename = this.username + "_DSCQS_result.txt";
		
		yt.actionClose.setEnabled(false);
		yt.enableContextMenu(false);
		yt.actionScore.trigger();
		yt.actionZoom100.trigger();
		yt.playbackToolBar.setDisabled(true);
		yt.menuBar.setDisabled(true);
		yt.mainToolBar.setDisabled(true);
		yt.statusBar.hide();
		yt.showMaximized();
		yt.setSetting("main/playbackloop", false);
		
		yt.scoreWindow.changeButtonName("Replay video", "Next", "Finish");
		
		yt.scoreWindow.onNext.connect(this, "nextVideo");
		yt.scoreWindow.onPrevious.connect(this, "prevousVideo");
		yt.scoreWindow.onFinish.connect(this, "finish");
		
		//enable rate Button, set number and their names
		yt.scoreWindow.createSlider(1,0,100,"left,right","Excellent,good,fair,poor,bad");
		//enable rate scale, set scale range and level names
		
		this.nextVideo();
	};
	
	this.finish = function() {
		yt.scoreWindow.onNext.disconnect(this, "nextVideo");
		yt.scoreWindow.onPrevious.disconnect(this, "prevousVideo");
		yt.scoreWindow.onFinish.disconnect(this, "finish");
		
		line = yt.scoreWindow.getCurSliderResults(this.videoList[this.current]);
		yt.scoreWindow.openResultsFile(this.resultfilename, 2);
		yt.scoreWindow.writeResultsFile(line);
		yt.scoreWindow.closeResultsFile();
		
		yt.actionScore.trigger();
		yt.closeAll();
		
		yt.actionClose.setEnabled(true);
		yt.enableContextMenu(true);
		
		yt.close();
	};
	
	this.openVideo = function() {
		playback = new playback_control()
		yt.lastFrameDisplayed.connect(playback, "lastFrameDisplayed");
		yt.setWindowTitle("Video " + (this.current+1) + " of " + this.videoList.length);
		yt.scoreWindow.initSlider();
		yt.scoreWindow.enableButtons(true, false, this.current==this.videoList.length-1);
		yt.infoMsg('', this.videoList[this.current][0]);
		yt.openFile(this.videoList[this.current][0]);
		while (playback.counter != 1) {
			sleep(1000)
		};
		yt.infoMsg('', 'Black_3s');
		playback.reset_counter();	
		yt.openFile(this.black_3s);
		while (playback.counter != 1) {
			sleep(1000)
		};
		yt.infoMsg('', this.videoList[this.current][1]);
		playback.reset_counter();	
		yt.openFile(this.videoList[this.current][1]);
		while (playback.counter != 1) {
			sleep(1000)
		};
	};
	
	this.nextVideo = function() {
		if (this.current==0)
		{
			line = yt.scoreWindow.getCurSliderResults(this.videoList[this.current]);
			yt.scoreWindow.openResultsFile(this.resultfilename, 0);
			yt.scoreWindow.writeResultsFile(line);
			yt.scoreWindow.closeResultsFile();
		}
		if (this.current==this.videoList.length-1) 
		{
			yt.infoMsg('Message','Already the last video! Please press the Finish Button');
			yt.scoreWindow.enableButtons(true, false, this.current==this.videoList.length-1);
			return;
		}
		
		if (this.current > 0) 
		{
			line = yt.scoreWindow.getCurSliderResults(this.videoList[this.current]);
			yt.scoreWindow.openResultsFile(this.resultfilename, 2);
			yt.scoreWindow.writeResultsFile(line);
			yt.scoreWindow.closeResultsFile();
		}
		
		yt.closeAll();
		
		this.current ++;
		this.openVideo();
	};

	this.prevousVideo = function() {
	
		yt.closeAll();
		
		this.openVideo();
	};
};