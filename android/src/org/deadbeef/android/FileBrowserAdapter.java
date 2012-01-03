package org.deadbeef.android;
import org.deadbeef.android.R;

import java.io.File;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Comparator;
import java.util.List;

import android.content.Context;
import android.view.LayoutInflater;
import android.view.View;
import android.view.ViewGroup;
import android.widget.BaseAdapter;
import android.widget.TextView;
import android.os.Environment;

public class FileBrowserAdapter extends BaseAdapter {
    private Context myContext;
    private List<String> files = new ArrayList<String> ();
    private List<String> history = new ArrayList<String> ();
    private String currentPath;

    public FileBrowserAdapter(Context context, String initPath) {
        super();
        this.myContext = context;
        setPath (initPath);
    }

    class MyComp implements Comparator {
     @Override
     public int compare(Object o1, Object o2) {
   File f1 = (File)o1;
   File f2 = (File)o2;
   String s1 = f1.toString ();
   String s2 = f2.toString ();
   if (!f1.isDirectory ()) {
    s1 = "~" + s1;
   }
   else {
    s1 = " " + s1;
   }
   if (!f2.isDirectory ()) {
    s2 = "~" + s2;
   }
   else {
    s2 = " " + s2;
   }
   return s1.compareToIgnoreCase(s2);
  }
    };

    public void setPathReal () {
     File currentDir = new File (currentPath);
     files.clear ();
     files.add ("..");
     File list[] = currentDir.listFiles ();
     if (list == null) {
      currentPath = Environment.getExternalStorageDirectory().getAbsolutePath();
      setPathReal ();
      return;
     }
     java.util.Arrays.sort(list, new MyComp());
     if (list != null) {
      for (File f : list) {
       if (f.isDirectory ()) {
        files.add ("<DIR> " + f.getName ());
       }
       else {
        files.add (f.getName ());
       }
      }
     }
     notifyDataSetChanged ();
    }


    public void setPath (String path) {
     history.add(currentPath);
     currentPath = path;
     setPathReal ();
    }

    public boolean goBack () {
     if (history.size() <= 1) {
      return false;
     }

     currentPath = history.get(history.size () - 1);
     history.remove(history.size () - 1);
     setPathReal ();
     return true;
    }

    public int getCount() {
     return files.size ();
    }

    public Object getItem(int position) {
     return files.get(position);
    }

    public long getItemId(int position) {
     return position;
    }

    public boolean hasStableIds() {
     return false;
    }

   public View getView(int position, View convertView, ViewGroup parent) {
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

   public boolean Clicked (int position) {
    if (position == 0) {
     // step one folder up
     File clickedFile = new File(currentPath);

     clickedFile = clickedFile.getParentFile();
     if (clickedFile == null) {
      return false;
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
    return true;
   }

   public String getPath () {
    return currentPath;
   }

}