package org.deadbeef.android;

import java.io.File;
import java.util.ArrayList;
import java.util.List;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;


public class FileBrowserAdapter extends BaseAdapter {
    private Context myContext;
    private List<String> files = new ArrayList<String> ();
    private String currentPath;

    public FileBrowserAdapter(Context context, String initPath) {
        super();
        this.myContext = context;
        setPath (initPath);
    }
    
    public void setPath (String path) {
    	currentPath = path;
    	File currentDir = new File (path); 
    	files.clear ();
    	files.add ("..");
    	File list[] = currentDir.listFiles ();
    	for (File f : list) {
    		if (f.isDirectory ()) {
    			files.add ("<DIR> " + f.getName ());
    		}
    		else {
    			files.add (f.getName ());
    		}
    	}
    }
    
    public int getCount() {
    	return files.size ();
    }
    
    public Object	 getItem(int position) {
    	return files.get(position);
    }
    
    public long	 getItemId(int position) {
    	return position;
    }
 
    public boolean	 hasStableIds() {
    	return false;
    }

   public View	 getView(int position, View convertView, ViewGroup parent) {
	    	View v = convertView;
	        if (v == null) {
	            LayoutInflater vi = (LayoutInflater)myContext.getSystemService(Context.LAYOUT_INFLATER_SERVICE);
	            v = vi.inflate(R.layout.file, null);
	        }
	        String o = files.get (position);

	        if (o != null && v != null) {                                
	            TextView tt = (TextView) v.findViewById(R.id.file);
	            if (tt != null) {
	                tt.setText(o);
	            }
	        }

	        return v;
	}
   
   public void Clicked (int position) {
	   if (position == 0) {
		   // step one folder up
		   File clickedFile = new File(currentPath);

		   clickedFile = clickedFile.getParentFile();
		   if (clickedFile == null) {
			   return;
		   }
		   setPath (clickedFile.getAbsolutePath());
	   }
	   else {
		   String pathName = files.get(position);
		   if (pathName.startsWith ("<DIR> ")) {
			   pathName = pathName.substring (6, pathName.length()); 
		   }
		   File clickedFile = new File(currentPath+'/'+pathName);
		   if (clickedFile.isDirectory ()) {
			   setPath (clickedFile.getAbsolutePath());
		   }
	   }
	   notifyDataSetChanged ();
   }

    public void AddFolder () {
        // add folder to playlist
        DeadbeefAPI.pl_add_folder (currentPath);
    }
    
    public void AddFolderAfter (int plt, int trk) {
        DeadbeefAPI.pl_insert_dir (plt, trk, currentPath);
    }
}
