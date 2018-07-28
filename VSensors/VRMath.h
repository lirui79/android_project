#pragma once

#include <cmath>
#include <memory>
#include <cstring>

namespace iris {

// Quaternion: q = q[0,1,2,3] * (1,i,j,k)
// or q = exp(t/2 (xi,yj,zk) ) = cos(t/2) + (xi+yj+zk)sin(t/2)
// rotate vector: v' = qvq^-1
// rotate with q1 then q2: q' = q2q1
// vector3: a vector with 3 dimension, such as position, velocity, etc.
// vector4: a vector with 4 dimension, used as quaternion for most cases.

const float GRAVITY = 9806.6f;

template <typename T>
inline T sqr(T a){return a*a;}

template <typename T>
class Math{
public:
	// Return: PI=3.141592653...
	static T pi(){
		return std::acos((T)-1);
	}
	
	// Args    :  x
	// x       :  number
	// Return  :  absolute value of x
	static T abs(T x){
		return x>0?x:-x;
	}

	// Args    :  x
	// x       :  number
	// Return  :  1/sqrt(x)
	static T rsqrt(T x){
		return 1/std::sqrt(x);
	}

	// Return  :  a smalll positive value --> 0
	static T eps(){
		return static_cast<T>(1E-7);
	}

	// Args    :  v, coeff, u
	// v       :  vector3, may be position, velocity, etc.
	// coeff   :  number, coefficient to multiply
	// u       :  vector3 stored, to be multiply coefficient elementally to v
	// i.e. 
	//       u = v*coeff
	static void vec_scale(const T *v, T coeff, T *u){
		u[0] = v[0]*coeff;
		u[1] = v[1]*coeff;
		u[2] = v[2]*coeff;
	}


	// Args    :  v, u
	// v       :  vector3
	// u       :  vector3 stored, elementally square
	// i.e.
	//       u_i = v_i*v_i, forall i
	static void vec_sqr(const T *v, T *u){
		u[0] = v[0]*v[0];
		u[1] = v[1]*v[1];
		u[2] = v[2]*v[2];
	}

	// Args    :  v, u
	// v       :  vector3
	// u       :  vector3 stored, as u = v
	static void vec_copy(const T *v, T *u){
		std::memcpy(u, v, sizeof(T)*3);
	}

	// Args    :  v, u, w
	// v, u    :  vector3 both
	// w       :  vector3 stored, add v & u
	// i.e. 
	//       w = v+u
	static void vec_add(const T *v, const T *u, T *w){
		T t[3];
		t[0] = v[0]+u[0];
		t[1] = v[1]+u[1];
		t[2] = v[2]+u[2];
		std::memcpy(w, t, sizeof(T)*3);
	}

	// Args    :  v, u, w
	// v, u    :  vector3 both
	// w       :  vector3 stored, sub v by u
	// i.e. 
	//       w = v-u
	static void vec_sub(const T *v, const T *u, T *w){
		T t[3];
		t[0] = v[0]-u[0];
		t[1] = v[1]-u[1];
		t[2] = v[2]-u[2];
		std::memcpy(w,t,sizeof(T)*3);
	}

	// Args    :  v, u
	// v, u    :  vector3 both
	// Return  :  dot product of v & u
	// i.e. 
	//       <v,u>
	static T vec_dot(const T *v, const T *u){
		return v[0]*u[0] + v[1]*u[1] + v[2]*u[2];
	}

	// Args    :  v, u, w
	// v, u    :  vector3 both
	// w       :  vector3 stored, cross product of v & u
	// i.e.
	//       w = vXu
	static void vec_cross(const T *v, const T *u, T *w){
		T t[3];
		t[0] = v[1]*u[2] - v[2]*u[1];
		t[1] = v[2]*u[0] - v[0]*u[2];
		t[2] = v[0]*u[1] - v[1]*u[0];
		std::memcpy(w, t, sizeof(T)*3);
	}

	// Args    :  v
	// v       :  vector3
	// Return  :  L2-norm of v
	// i.e.
	//       <v,v>
	static T vec_l2(const T *v){
		return vec_dot(v,v);
	}

	// Args    :  v, u
	// v       :  vector3
	// u       :  vector3 stored, normalization of v
	// i.e.
	//       u = v/|v|
	static void vec_norm(const T *v, T *u){
		T d = vec_l2(v);
#ifdef _DEBUG
		if (d<eps())
			bug("Dangerous vec_norm");
#endif
		d = rsqrt(d);
		vec_scale(v, d, u);
	}

	// Args    :  v, u
	// v, u    :  vector3 both
	// Return  :  cosine of angle between v & u
	// i.e.
	//       <v,u> / (|v||u|)
	static T vec_cos(const T *v, const T *u){
		T ct = vec_dot(v, u) * rsqrt(vec_l2(v)*vec_l2(u));
		return ct;
	}
	
	// Args    :  v, q, u
	// v       :  vector3
	// q       :  vector4, a quaternion
	// u       :  vector3 stored, rotate v with quaternion q
	// i.e.
	//       u = q v q^-1
	static void vec_rotate(const T *v, const T *q, T *u){
		T t[3], s[3];
		vec_cross(q+1,v,t);
		vec_scale(t,2,t);
		vec_cross(q+1,t,s);
		vec_scale(t,q[0],t);
		vec_add(v,s,u);
		vec_add(u,t,u);
	}

	// Args    :  v, q, u
	// v       :  vector3
	// q       :  vector4, a quaternion
	// u       :  vector3 stored, rotate v with quaternion q^-1
	// i.e.
	//       u = q^-1 v q
	static void vec_rotate_inv(const T *v, const T *q, T *u){
		T t[3], s[3];
		vec_cross(v,q+1,t);
		vec_scale(t,2,t);
		vec_cross(t,q+1,s);
		vec_scale(t,q[0],t);
		vec_add(v,s,u);
		vec_add(u,t,u);
	}

	// Args    :  q
	// q       :  vector4, set to identity
	// i.e.
	//       q = {1, 0, 0, 0}
	static void qua_identity(T *q){
		q[0] = 1;
		q[1] = q[2] = q[3] = 0;
	}

	// Args    :  p, q
	// p       :  vector4
	// q       :  vector4 stored, copy from p
	// i.e.
	//       q = p
	static void qua_copy(const T *p, T *q){
		std::memcpy(q, p, sizeof(T) *4);
	}

	// Args    :  p, q
	// p       :  vector4
	// q       :  vector4 stored, inverse of p
	// i.e.
	//       q = p^-1
	static void qua_inv(const T *p, T *q){
		q[0] = p[0];
		q[1] = -p[1], q[2] = -p[2], q[3] = -p[3];
	}

	// Args    :  p, q, r
	// p, q    :  vector4 both
	// r       :  vector4 stored, multiply of p & q
	// i.e.
	//       r = p q
	static void qua_mul(const T *p, const T *q, T *r){
		T t[4];
		t[0] = p[0]*q[0] - p[1]*q[1] - p[2]*q[2] - p[3]*q[3];
		t[1] = p[1]*q[0] + p[0]*q[1] - p[3]*q[2] + p[2]*q[3];
		t[2] = p[2]*q[0] + p[3]*q[1] + p[0]*q[2] - p[1]*q[3];
		t[3] = p[3]*q[0] - p[2]*q[1] + p[1]*q[2] + p[0]*q[3];
		std::memcpy(r,t, sizeof(T)*4);
	}

	// Args    :  p, q
	// p       :  vector4
	// q       :  vector4 stored, normalization of p
	// i.e.
	//       q = p/|p|
	static void qua_norm(const T *p, T *q){
		T d = p[0]*p[0]+p[1]*p[1]+p[2]*p[2]+p[3]*p[3];
		if(d<eps()){
#ifdef _DEBUG
			bug("Dangerous qua_norm");
#endif
			qua_identity(q);
			return;
		}
		//d = (d==0)?0:(p[0]<0)?-rsqrt(d):rsqrt(d);  wtf....
		if (abs(1.0- d)< eps()){
			d = 2.0/(1.0 + d);
		}
		else{
			d = rsqrt(d);
		}
		q[0] = p[0]*d;
		q[1] = p[1]*d;
		q[2] = p[2]*d;
		q[3] = p[3]*d;
	}

	// Args    :  axis, theta, p
	// axis    :  vector3, rotation axis
	// theta   :  number, a rotation angle along axis
	// p       :  vector4 stored, represent rotation above
	// i.e.
	//       p = exp[theta/2 (axis)]
	static void qua_by_axis_angle(const T *axis, T theta, T *p){
		if (theta==0){
			qua_identity(p);
			return;
		}

		T norm_axis[3];
		vec_norm(axis, norm_axis);
		T s = sin(theta/2);
		T c = cos(theta/2);
		p[0] = c;
		vec_scale(norm_axis, s, p+1);
	}

	// Args    :  axis, ct, p
	// axis    :  vector3, rotation axis
	// ct      :  number, cosine of rotation angle
	// p       :  vector4 stored, represent rotation above
	static void qua_by_cos(const T *axis, T ct, T *p){
		if (abs(ct-1.f)<eps() || ct>=1.f ||ct<=-1.f){
			qua_identity(p);
			return;
		}
		T c = sqrt((1+ct)/2 + eps());
		T s = sqrt((1-ct)/2 + eps());
		p[0] = c;
		vec_scale(axis, s, p+1);
	}

};
typedef Math<float> Mathf;

}