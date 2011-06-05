// File generated by hadoop record compiler. Do not edit.
package org.apache.zookeeper.txn;

import java.util.*;
import org.apache.jute.*;
public class ErrorTxn implements Record {
  private int err;
  public ErrorTxn() {
  }
  public ErrorTxn(
        int err) {
    this.err=err;
  }
  public int getErr() {
    return err;
  }
  public void setErr(int m_) {
    err=m_;
  }
  public void serialize(OutputArchive a_, String tag) throws java.io.IOException {
    a_.startRecord(this,tag);
    a_.writeInt(err,"err");
    a_.endRecord(this,tag);
  }
  public void deserialize(InputArchive a_, String tag) throws java.io.IOException {
    a_.startRecord(tag);
    err=a_.readInt("err");
    a_.endRecord(tag);
}
  public String toString() {
    try {
      java.io.ByteArrayOutputStream s =
        new java.io.ByteArrayOutputStream();
      CsvOutputArchive a_ = 
        new CsvOutputArchive(s);
      a_.startRecord(this,"");
    a_.writeInt(err,"err");
      a_.endRecord(this,"");
      return new String(s.toByteArray(), "UTF-8");
    } catch (Throwable ex) {
      ex.printStackTrace();
    }
    return "ERROR";
  }
  public void write(java.io.DataOutput out) throws java.io.IOException {
    BinaryOutputArchive archive = new BinaryOutputArchive(out);
    serialize(archive, "");
  }
  public void readFields(java.io.DataInput in) throws java.io.IOException {
    BinaryInputArchive archive = new BinaryInputArchive(in);
    deserialize(archive, "");
  }
  public int compareTo (Object peer_) throws ClassCastException {
    if (!(peer_ instanceof ErrorTxn)) {
      throw new ClassCastException("Comparing different types of records.");
    }
    ErrorTxn peer = (ErrorTxn) peer_;
    int ret = 0;
    ret = (err == peer.err)? 0 :((err<peer.err)?-1:1);
    if (ret != 0) return ret;
     return ret;
  }
  public boolean equals(Object peer_) {
    if (!(peer_ instanceof ErrorTxn)) {
      return false;
    }
    if (peer_ == this) {
      return true;
    }
    ErrorTxn peer = (ErrorTxn) peer_;
    boolean ret = false;
    ret = (err==peer.err);
    if (!ret) return ret;
     return ret;
  }
  public int hashCode() {
    int result = 17;
    int ret;
    ret = (int)err;
    result = 37*result + ret;
    return result;
  }
  public static String signature() {
    return "LErrorTxn(i)";
  }
}
