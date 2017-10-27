//
// singleton.h
//
//  Created on: 2016-05-05
//      Author: zjg
//

#ifndef SINGLETON_H_
#define SINGLETON_H_

template<class T>
class singleton
{
public:
	static T& instance()
	{ 
		if(NULL == m_pInstance)
		{
			m_pInstance = new T;
		}
		return *m_pInstance;
	}
	static void uninstance()
	{
		if (NULL != m_pInstance)
		{
			delete m_pInstance;
			m_pInstance = NULL;
		}
	}
protected:
	singleton(){};
private:
	static T* m_pInstance;
 };

template<class T>
T*	singleton<T>::m_pInstance = NULL;

#endif // SINGLETON_H_
